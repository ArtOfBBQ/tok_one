#include "stdio.h"
#include "box.h"
#include "software_renderer.h"

int main() {
    zPolygon * box = get_box();
    
    z_sort(box);
}

