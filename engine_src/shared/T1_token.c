#include "T1_token.h"

#include "T1_assert.h"
#include "T1_mem.h"
#include "T1_std.h"

/*
How it works:
Literals and the starts of delimited rules go into a trie. A run is alive
while every byte so far was in its sets, which a bitmask of the runs can
track. So a state of the merged machine is (trie node or none, alive runs),
and T1_token_build() finds every reachable state with one table row each.
Bytes that behave the same in every rule share a column (a byte class),
which keeps the table small.

At every position, T1_token_run() follows the table until it hits the dead
state, remembering the longest match it passed (a delimited rule's match
reaches up to its stop text, found by scanning on).
*/

#define TOKENIZERS_CAP 16
#define RUNS_CAP 32
#define NONE UINT32_MAX
#define DEAD_STATE 0
#define START_STATE 1
#define TRIE_ROOT 0

typedef u8 RuleType;
enum {
    RULE_LITERAL,
    RULE_RUN,
    RULE_DELIMITED,
};

typedef struct {
    c8 * stop; // delimited only, owned
    u32 kind;
    u32 start_size; // delimited only
    u32 stop_size;
    c8 escape;
    u8 stop_in_token;
    RuleType type;
    u8 discard;
} Rule;

typedef struct {
    u32 first_child;
    u32 next_sibling;
    u32 literal_rule;   // NONE or the first literal ending here
    u32 delimited_rule; // NONE or the first delimited rule starting here
    u8 byte;
} TrieNode;

typedef struct {
    u32 rule; // NONE for unmatched bytes
    u32 offset;
    u32 size;
    u32 line;
} Token;

typedef struct {
    Rule * rules;
    TrieNode * nodes;
    u64 run_first[RUNS_CAP][4]; // 256-bit byte sets
    u64 run_rest[RUNS_CAP][4];
    u32 run_rule[RUNS_CAP];
    u32 rules_size;
    u32 rules_cap;
    u32 nodes_size;
    u32 nodes_cap;
    u32 runs_size;

    // made by T1_token_build()
    u16 * next; // states_size rows of classes_size
    u32 * state_accept;    // best literal or run rule, or NONE
    u32 * state_delimited; // delimited rule to try, or NONE
    u32 states_size;
    u32 classes_size;
    u8 byte_class[256];
    b8 built;

    // made by T1_token_run()
    const c8 * text;
    Token * tokens;
    u32 text_size;
    u32 end_line;
    u32 tokens_size;
    u32 tokens_cap;
    
    // made by the first T1_token_value() after a run
    c8 * values;         // each token's value and a 0
    u32 * value_offsets; // where in values
} Tokenizer;

static Tokenizer * tokenizers[TOKENIZERS_CAP];

static Tokenizer * get(const u32 tz) {
    T1_assert(tz > 0 && tz <= TOKENIZERS_CAP);
    T1_assert(tokenizers[tz - 1] != NULL);
    return tokenizers[tz - 1];
}

// Grows an array allocated with T1_mem_malloc_managed (or NULL)
static void * grow(void * old, const u64 old_size, const u64 new_size) {
    T1_assert(new_size > old_size);
    void * new = T1_mem_malloc_managed(new_size);
    if (old != NULL) {
        T1_std_memcpy(new, old, old_size);
        T1_mem_free_managed(old);
    }
    return new;
}

static void free_if_set(void ** ptr) {
    if (*ptr != NULL) {
        T1_mem_free_managed(*ptr);
        *ptr = NULL;
    }
}

static void free_values(Tokenizer * t) {
    free_if_set((void **)&t->values);
    free_if_set((void **)&t->value_offsets);
}

static void free_build(Tokenizer * t) {
    free_if_set((void **)&t->next);
    free_if_set((void **)&t->state_accept);
    free_if_set((void **)&t->state_delimited);
    t->states_size = 0;
    t->built = 0;
}

static u32 new_node(Tokenizer * t, const u8 byte) {
    if (t->nodes_size >= t->nodes_cap) {
        u32 new_cap = t->nodes_cap * 2;
        t->nodes = grow(
            t->nodes,
            sizeof(TrieNode) * t->nodes_cap,
            sizeof(TrieNode) * new_cap);
        t->nodes_cap = new_cap;
    }
    TrieNode * n = &t->nodes[t->nodes_size];
    n->first_child = NONE;
    n->next_sibling = NONE;
    n->literal_rule = NONE;
    n->delimited_rule = NONE;
    n->byte = byte;
    t->nodes_size += 1;
    return t->nodes_size - 1;
}

static u32 find_child(const Tokenizer * t, const u32 node, const u8 byte) {
    for (
        u32 c = t->nodes[node].first_child;
        c != NONE;
        c = t->nodes[c].next_sibling)
    {
        if (t->nodes[c].byte == byte) { return c; }
    }
    return NONE;
}

// The trie node for text, made if needed
static u32 trie_insert(Tokenizer * t, const c8 * text) {
    T1_assert(text != NULL && text[0] != '\0');
    u32 node = TRIE_ROOT;
    for (u32 i = 0; text[i] != '\0'; i++) {
        u32 child = find_child(t, node, (u8)text[i]);
        if (child == NONE) {
            child = new_node(t, (u8)text[i]);
            t->nodes[child].next_sibling = t->nodes[node].first_child;
            t->nodes[node].first_child = child;
        }
        node = child;
    }
    return node;
}

static u32 new_rule(
    Tokenizer * t,
    const RuleType type,
    const u32 kind,
    const u8 discard)
{
    T1_assert(kind != T1_TOKEN_UNMATCHED);
    free_build(t); // rules changed, so T1_token_build() again

    if (t->rules_size >= t->rules_cap) {
        u32 new_cap = t->rules_cap * 2;
        t->rules = grow(
            t->rules,
            sizeof(Rule) * t->rules_cap,
            sizeof(Rule) * new_cap);
        t->rules_cap = new_cap;
    }
    Rule * r = &t->rules[t->rules_size];
    T1_std_memset(r, 0, sizeof(Rule));
    r->type = type;
    r->kind = kind;
    r->discard = discard;
    t->rules_size += 1;
    return t->rules_size - 1;
}

static void set_add(u64 set[4], const u8 byte) {
    set[byte >> 6] |= (1ULL << (byte & 63));
}

static b8 set_has(const u64 set[4], const u8 byte) {
    return (set[byte >> 6] >> (byte & 63)) & 1;
}

// "A-Za-z_" style: ranges and single bytes, '^' first negates
static void parse_set(const c8 * text, u64 set[4]) {
    T1_assert(text != NULL);
    set[0] = set[1] = set[2] = set[3] = 0;

    u32 i = 0;
    b8 negate = text[0] == '^';
    if (negate) { i = 1; }

    while (text[i] != '\0') {
        u8 lo = (u8)text[i];
        if (text[i + 1] == '-' && text[i + 2] != '\0') {
            u8 hi = (u8)text[i + 2];
            T1_assert(lo <= hi);
            for (u32 b = lo; b <= hi; b++) { set_add(set, (u8)b); }
            i += 3;
        } else {
            set_add(set, lo);
            i += 1;
        }
    }

    if (negate) {
        for (u32 j = 0; j < 4; j++) { set[j] = ~set[j]; }
    }
}

u32 T1_token_new(void) {
    u32 slot = NONE;
    for (u32 i = 0; i < TOKENIZERS_CAP; i++) {
        if (tokenizers[i] == NULL) { slot = i; break; }
    }
    T1_assert(slot != NONE); // more than TOKENIZERS_CAP at once
    if (slot == NONE) { T1_crash("T1_token_new(): too many tokenizers"); }

    Tokenizer * t = T1_mem_malloc_managed(sizeof(Tokenizer));
    T1_std_memset(t, 0, sizeof(Tokenizer));

    t->rules_cap = 64;
    t->rules = T1_mem_malloc_managed(sizeof(Rule) * t->rules_cap);
    t->nodes_cap = 256;
    t->nodes = T1_mem_malloc_managed(sizeof(TrieNode) * t->nodes_cap);
    new_node(t, 0); // TRIE_ROOT
    t->tokens_cap = 1024;
    t->tokens = T1_mem_malloc_managed(sizeof(Token) * t->tokens_cap);

    tokenizers[slot] = t;
    return slot + 1;
}

void T1_token_delete(const u32 tz) {
    Tokenizer * t = get(tz);
    free_build(t);
    free_values(t);
    for (u32 i = 0; i < t->rules_size; i++) {
        free_if_set((void **)&t->rules[i].stop);
    }
    T1_mem_free_managed(t->rules);
    T1_mem_free_managed(t->nodes);
    T1_mem_free_managed(t->tokens);
    T1_mem_free_managed(t);
    tokenizers[tz - 1] = NULL;
}

void T1_token_add_literal(
    const u32 tz,
    const c8 * text,
    const u32 kind,
    const u8 discard)
{
    Tokenizer * t = get(tz);
    u32 rule = new_rule(t, RULE_LITERAL, kind, discard);
    u32 node = trie_insert(t, text);
    if (t->nodes[node].literal_rule == NONE) {
        t->nodes[node].literal_rule = rule;
    }
}

void T1_token_add_run(
    const u32 tz,
    const c8 * first_set,
    const c8 * rest_set,
    const u32 kind,
    const u8 discard)
{
    Tokenizer * t = get(tz);
    T1_assert(t->runs_size < RUNS_CAP);
    if (t->runs_size >= RUNS_CAP) {
        T1_crash("T1_token_add_run(): more than 32 runs");
    }

    u32 rule = new_rule(t, RULE_RUN, kind, discard);
    parse_set(first_set, t->run_first[t->runs_size]);
    parse_set(rest_set, t->run_rest[t->runs_size]);
    t->run_rule[t->runs_size] = rule;
    t->runs_size += 1;
}

void T1_token_add_delimited(
    const u32 tz,
    const c8 * start,
    const c8 * stop,
    const c8 escape,
    const u8 stop_in_token,
    const u32 kind,
    const u8 discard)
{
    Tokenizer * t = get(tz);
    T1_assert(stop != NULL && stop[0] != '\0');

    u32 rule = new_rule(t, RULE_DELIMITED, kind, discard);
    Rule * r = &t->rules[rule];
    r->start_size = (u32)T1_std_strlen(start);
    r->stop_size = (u32)T1_std_strlen(stop);
    r->stop = T1_mem_malloc_managed(r->stop_size + 1);
    T1_std_memcpy(r->stop, stop, r->stop_size + 1);
    r->escape = escape;
    r->stop_in_token = stop_in_token;

    u32 node = trie_insert(t, start);
    if (t->nodes[node].delimited_rule == NONE) {
        t->nodes[node].delimited_rule = rule;
    }
}

/*
Two bytes share a class if no literal (or delimited start) contains either
and every run treats them the same
*/
static void build_byte_classes(Tokenizer * t, u8 class_rep[256]) {
    b8 in_trie[256];
    T1_std_memset(in_trie, 0, 256);
    for (u32 n = 1; n < t->nodes_size; n++) {
        in_trie[t->nodes[n].byte] = 1;
    }

    u64 class_sig[256];
    b8 class_in_trie[256];
    t->classes_size = 0;

    for (u32 b = 0; b < 256; b++) {
        u64 sig = 0;
        for (u32 r = 0; r < t->runs_size; r++) {
            sig |= (u64)set_has(t->run_first[r], (u8)b) << r;
            sig |= (u64)set_has(t->run_rest[r], (u8)b) << (32 + r);
        }

        u32 found = NONE;
        if (!in_trie[b]) {
            for (u32 c = 0; c < t->classes_size; c++) {
                if (!class_in_trie[c] && class_sig[c] == sig) {
                    found = c;
                    break;
                }
            }
        }

        if (found == NONE) {
            found = t->classes_size;
            class_sig[found] = sig;
            class_in_trie[found] = in_trie[b];
            class_rep[found] = (u8)b;
            t->classes_size += 1;
        }
        t->byte_class[b] = (u8)found;
    }
}

void T1_token_build(const u32 tz) {
    Tokenizer * t = get(tz);
    free_build(t);

    u8 class_rep[256];
    build_byte_classes(t, class_rep);

    /*
    The states found so far, (node, mask) each, node being nodes_size for
    "no trie node". node_first_state and state_next_same_node chain the
    states by node, to find a state that already exists.
    */
    u32 none_node = t->nodes_size;
    u32 * node_first_state =
        T1_mem_malloc_managed(sizeof(u32) * (t->nodes_size + 1));
    for (u32 n = 0; n <= t->nodes_size; n++) { node_first_state[n] = NONE; }

    u32 states_cap = 64;
    u32 * state_node = T1_mem_malloc_managed(sizeof(u32) * states_cap);
    u32 * state_mask = T1_mem_malloc_managed(sizeof(u32) * states_cap);
    u32 * state_next_same_node =
        T1_mem_malloc_managed(sizeof(u32) * states_cap);
    t->next = T1_mem_malloc_managed(
        sizeof(u16) * states_cap * t->classes_size);
    t->state_accept = T1_mem_malloc_managed(sizeof(u32) * states_cap);
    t->state_delimited = T1_mem_malloc_managed(sizeof(u32) * states_cap);

    // DEAD_STATE, then START_STATE
    for (u32 s = 0; s < 2; s++) {
        state_node[s] = s == DEAD_STATE ? none_node : TRIE_ROOT;
        state_mask[s] = 0;
        state_next_same_node[s] = NONE;
        t->state_accept[s] = NONE;
        t->state_delimited[s] = NONE;
    }
    node_first_state[TRIE_ROOT] = START_STATE;
    t->states_size = 2;
    for (u32 c = 0; c < t->classes_size; c++) {
        t->next[DEAD_STATE * t->classes_size + c] = DEAD_STATE;
    }

    // states_size grows while we fill in each state's row
    for (u32 s = START_STATE; s < t->states_size; s++) {
        for (u32 c = 0; c < t->classes_size; c++) {
            u8 b = class_rep[c];

            u32 node = state_node[s] == none_node ?
                NONE : find_child(t, state_node[s], b);

            u32 mask = 0;
            for (u32 r = 0; r < t->runs_size; r++) {
                if (s == START_STATE) {
                    mask |= (u32)set_has(t->run_first[r], b) << r;
                } else if ((state_mask[s] >> r) & 1) {
                    mask |= (u32)set_has(t->run_rest[r], b) << r;
                }
            }

            if (node == NONE && mask == 0) {
                t->next[s * t->classes_size + c] = DEAD_STATE;
                continue;
            }
            if (node == NONE) { node = none_node; }

            u32 target = node_first_state[node];
            while (target != NONE && state_mask[target] != mask) {
                target = state_next_same_node[target];
            }

            if (target == NONE) {
                T1_assert(t->states_size < UINT16_MAX);
                if (t->states_size >= UINT16_MAX) {
                    T1_crash("T1_token_build(): too many states");
                }

                if (t->states_size >= states_cap) {
                    u32 new_cap = states_cap * 2;
                    state_node = grow(state_node,
                        sizeof(u32) * states_cap, sizeof(u32) * new_cap);
                    state_mask = grow(state_mask,
                        sizeof(u32) * states_cap, sizeof(u32) * new_cap);
                    state_next_same_node = grow(state_next_same_node,
                        sizeof(u32) * states_cap, sizeof(u32) * new_cap);
                    t->state_accept = grow(t->state_accept,
                        sizeof(u32) * states_cap, sizeof(u32) * new_cap);
                    t->state_delimited = grow(t->state_delimited,
                        sizeof(u32) * states_cap, sizeof(u32) * new_cap);
                    t->next = grow(t->next,
                        sizeof(u16) * states_cap * t->classes_size,
                        sizeof(u16) * new_cap * t->classes_size);
                    states_cap = new_cap;
                }

                target = t->states_size;
                t->states_size += 1;
                state_node[target] = node;
                state_mask[target] = mask;
                state_next_same_node[target] = node_first_state[node];
                node_first_state[node] = target;

                // equally long matches go to the rule registered first
                u32 accept = NONE;
                u32 delimited = NONE;
                if (node != none_node) {
                    accept = t->nodes[node].literal_rule;
                    delimited = t->nodes[node].delimited_rule;
                }
                for (u32 r = 0; r < t->runs_size; r++) {
                    if (((mask >> r) & 1) && t->run_rule[r] < accept) {
                        accept = t->run_rule[r];
                    }
                }
                t->state_accept[target] = accept;
                t->state_delimited[target] = delimited;
            }

            t->next[s * t->classes_size + c] = (u16)target;
        }
    }

    T1_mem_free_managed(node_first_state);
    T1_mem_free_managed(state_node);
    T1_mem_free_managed(state_mask);
    T1_mem_free_managed(state_next_same_node);
    t->built = 1;
}

// Where a delimited token whose body starts at i ends
static u32 delimited_end(
    const Rule * r,
    const c8 * text,
    const u32 text_size,
    u32 i)
{
    while (i < text_size) {
        if (r->escape != '\0' && text[i] == r->escape) {
            i += 2;
            continue;
        }
        if (
            i + r->stop_size <= text_size &&
            T1_std_are_equal_strings_of_length(text + i, r->stop, r->stop_size))
        {
            return r->stop_in_token ? i + r->stop_size : i;
        }
        i += 1;
    }
    return text_size;
}

static void push_token(
    Tokenizer * t,
    const u32 rule,
    const u32 offset,
    const u32 size,
    const u32 line)
{
    if (t->tokens_size >= t->tokens_cap) {
        u32 new_cap = t->tokens_cap * 2;
        t->tokens = grow(
            t->tokens,
            sizeof(Token) * t->tokens_cap,
            sizeof(Token) * new_cap);
        t->tokens_cap = new_cap;
    }
    Token * tok = &t->tokens[t->tokens_size];
    tok->rule = rule;
    tok->offset = offset;
    tok->size = size;
    tok->line = line;
    t->tokens_size += 1;
}

void T1_token_run(
    const u32 tz,
    const c8 * text,
    const u64 text_size_u64)
{
    Tokenizer * t = get(tz);
    T1_assert(t->built); // T1_token_build() after the last rule
    T1_assert(text != NULL || text_size_u64 == 0);
    T1_assert(text_size_u64 < UINT32_MAX);

    const u32 text_size = (u32)text_size_u64;
    const u32 classes_size = t->classes_size;
    const u16 * next = t->next;

    t->text = text;
    t->text_size = text_size;
    t->tokens_size = 0;
    free_values(t);

    u32 pos = 0;
    u32 line = 1;
    while (pos < text_size) {
        u32 best_end = pos;
        u32 best_rule = NONE;

        u32 state = START_STATE;
        for (u32 i = pos; i < text_size; ) {
            state = next[state * classes_size + t->byte_class[(u8)text[i]]];
            if (state == DEAD_STATE) { break; }
            i += 1;

            u32 accept = t->state_accept[state];
            if (accept != NONE) {
                // i only grows, so a later match is a longer one
                best_end = i;
                best_rule = accept;
            }

            u32 delimited = t->state_delimited[state];
            if (delimited != NONE) {
                u32 end = delimited_end(
                    &t->rules[delimited], text, text_size, i);
                if (
                    end > best_end ||
                    (end == best_end && delimited < best_rule))
                {
                    best_end = end;
                    best_rule = delimited;
                }
            }
        }

        if (best_rule == NONE) {
            // a byte no rule matches, merged with unmatched ones before it
            Token * last = t->tokens_size > 0 ?
                &t->tokens[t->tokens_size - 1] : NULL;
            if (
                last != NULL &&
                last->rule == NONE &&
                last->offset + last->size == pos)
            {
                last->size += 1;
            } else {
                push_token(t, NONE, pos, 1, line);
            }
            best_end = pos + 1;
        } else if (!t->rules[best_rule].discard) {
            push_token(t, best_rule, pos, best_end - pos, line);
        }

        for (u32 i = pos; i < best_end; i++) {
            line += text[i] == '\n';
        }
        pos = best_end;
    }
    t->end_line = line;
}

u32 T1_token_count(const u32 tz) {
    return get(tz)->tokens_size;
}

u32 T1_token_kind(const u32 tz, const u32 i) {
    Tokenizer * t = get(tz);
    if (i >= t->tokens_size) { return T1_TOKEN_END; }
    u32 rule = t->tokens[i].rule;
    return rule == NONE ? T1_TOKEN_UNMATCHED : t->rules[rule].kind;
}

const c8 * T1_token_text(const u32 tz, const u32 i) {
    Tokenizer * t = get(tz);
    if (i >= t->tokens_size) { return t->text + t->text_size; }
    return t->text + t->tokens[i].offset;
}

u32 T1_token_size(const u32 tz, const u32 i) {
    Tokenizer * t = get(tz);
    return i < t->tokens_size ? t->tokens[i].size : 0;
}

u32 T1_token_line(const u32 tz, const u32 i) {
    Tokenizer * t = get(tz);
    return i < t->tokens_size ? t->tokens[i].line : t->end_line;
}

// A delimited token's text between start and stop, other tokens' whole text
static void value_range(
    const Tokenizer * t,
    const u32 i,
    const c8 ** text,
    u32 * size)
{
    if (i >= t->tokens_size) {
        *text = t->text + t->text_size;
        *size = 0;
        return;
    }
    const Token * tok = &t->tokens[i];
    *text = t->text + tok->offset;
    *size = tok->size;
    if (tok->rule == NONE || t->rules[tok->rule].type != RULE_DELIMITED) {
        return;
    }
    
    const Rule * r = &t->rules[tok->rule];
    u32 to = tok->size;
    if (
        r->stop_in_token &&
        to >= r->start_size + r->stop_size &&
        T1_std_are_equal_strings_of_length(
            *text + to - r->stop_size, r->stop, r->stop_size))
    {
        to -= r->stop_size; // it ends with stop, not with the text
    }
    *text += r->start_size;
    *size = to - r->start_size;
}

const c8 * T1_token_value(const u32 tz, const u32 i) {
    Tokenizer * t = get(tz);
    if (i >= t->tokens_size) { return ""; }
    
    if (t->values == NULL) {
        u64 values_size = 0;
        for (u32 j = 0; j < t->tokens_size; j++) {
            values_size += t->tokens[j].size + 1;
        }
        t->values = T1_mem_malloc_managed(values_size);
        t->value_offsets =
            T1_mem_malloc_managed(sizeof(u32) * t->tokens_size);
        
        u32 at = 0;
        for (u32 j = 0; j < t->tokens_size; j++) {
            const c8 * text;
            u32 size;
            value_range(t, j, &text, &size);
            t->value_offsets[j] = at;
            T1_std_memcpy(t->values + at, text, size);
            t->values[at + size] = '\0';
            at += size + 1;
        }
    }
    return t->values + t->value_offsets[i];
}

b8 T1_token_equals(const u32 tz, const u32 i, const c8 * str) {
    const c8 * text;
    u32 size;
    value_range(get(tz), i, &text, &size);
    for (u32 j = 0; j < size; j++) {
        if (str[j] != text[j]) { return 0; } // also stops at str's end
    }
    return str[size] == '\0';
}

void T1_token_copy(
    const u32 tz,
    const u32 i,
    c8 * recip,
    const u32 recip_cap)
{
    T1_assert(recip_cap > 0);
    const c8 * text;
    u32 size;
    value_range(get(tz), i, &text, &size);
    if (size > recip_cap - 1) { size = recip_cap - 1; }
    T1_std_memcpy(recip, text, size);
    recip[size] = '\0';
}

f64 T1_token_parse_f64(const u32 tz, const u32 i, u8 * good) {
    *good = 0;
    const c8 * s;
    u32 n;
    value_range(get(tz), i, &s, &n);

    u32 at = 0;
    b8 negative = 0;
    if (at < n && (s[at] == '-' || s[at] == '+')) {
        negative = s[at] == '-';
        at += 1;
    }

    // up to 19 significant digits fit a u64, later ones only scale
    u64 mantissa = 0;
    u32 mantissa_digits = 0;
    s32 exp10 = 0;
    u32 digits = 0;
    while (at < n && s[at] >= '0' && s[at] <= '9') {
        if (mantissa_digits < 19) {
            mantissa = mantissa * 10 + (u64)(s[at] - '0');
            mantissa_digits += mantissa > 0;
        } else {
            exp10 += 1;
        }
        digits += 1;
        at += 1;
    }
    if (at < n && s[at] == '.') {
        at += 1;
        while (at < n && s[at] >= '0' && s[at] <= '9') {
            if (mantissa_digits < 19) {
                mantissa = mantissa * 10 + (u64)(s[at] - '0');
                mantissa_digits += mantissa > 0;
                exp10 -= 1;
            }
            digits += 1;
            at += 1;
        }
    }
    if (digits == 0) { return 0.0; }

    if (at < n && (s[at] == 'e' || s[at] == 'E')) {
        at += 1;
        b8 exp_negative = 0;
        if (at < n && (s[at] == '-' || s[at] == '+')) {
            exp_negative = s[at] == '-';
            at += 1;
        }
        s32 exp = 0;
        u32 exp_digits = 0;
        while (at < n && s[at] >= '0' && s[at] <= '9') {
            if (exp < 100000) { exp = exp * 10 + (s[at] - '0'); }
            exp_digits += 1;
            at += 1;
        }
        if (exp_digits == 0) { return 0.0; }
        exp10 += exp_negative ? -exp : exp;
    }
    if (at != n) { return 0.0; }

    // not correctly rounded in the last bit, fine for assets
    f64 result = (f64)mantissa;
    if (mantissa != 0) {
        s32 e = exp10 < 0 ? -exp10 : exp10;
        if (e > 400) { e = 400; }
        f64 scale = 1.0;
        f64 power = 10.0;
        while (e > 0) {
            if (e & 1) { scale *= power; }
            power *= power;
            e >>= 1;
        }
        result = exp10 < 0 ? result / scale : result * scale;
    }

    *good = 1;
    return negative ? -result : result;
}

s64 T1_token_parse_s64(const u32 tz, const u32 i, u8 * good) {
    *good = 0;
    const c8 * s;
    u32 n;
    value_range(get(tz), i, &s, &n);

    u32 at = 0;
    b8 negative = 0;
    if (at < n && (s[at] == '-' || s[at] == '+')) {
        negative = s[at] == '-';
        at += 1;
    }

    u64 base = 10;
    if (at + 1 < n && s[at] == '0' && (s[at + 1] == 'x' || s[at + 1] == 'X')) {
        base = 16;
        at += 2;
    }

    // the largest magnitude: INT64_MAX, or one more if negative
    const u64 limit = (u64)INT64_MAX + negative;
    u64 value = 0;
    u32 digits = 0;
    for (; at < n; at++) {
        u64 d;
        c8 c = s[at];
        if (c >= '0' && c <= '9') {
            d = (u64)(c - '0');
        } else if (base == 16 && c >= 'a' && c <= 'f') {
            d = (u64)(c - 'a' + 10);
        } else if (base == 16 && c >= 'A' && c <= 'F') {
            d = (u64)(c - 'A' + 10);
        } else {
            return 0;
        }
        if (value > (limit - d) / base) { return 0; } // overflow
        value = value * base + d;
        digits += 1;
    }
    if (digits == 0) { return 0; }

    *good = 1;
    if (negative) {
        return value == (u64)INT64_MAX + 1 ? INT64_MIN : -(s64)value;
    }
    return (s64)value;
}
