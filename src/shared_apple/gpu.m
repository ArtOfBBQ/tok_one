#import "gpu.h"

MetalKitViewDelegate * apple_gpu_delegate = NULL;

@implementation MetalKitViewDelegate
{
    NSUInteger _currentFrameIndex;
}

- (void)
    configureMetalWithDevice: (id<MTLDevice>)metal_device
    andPixelFormat: (MTLPixelFormat)pixel_format
    fromFolder: (NSString *)shader_lib_filepath
{
    _currentFrameIndex = 0;
    
    _command_queue = [metal_device newCommandQueue];
    
    NSError *Error = NULL;
    
    id<MTLLibrary> shader_library =
        [metal_device
            newLibraryWithFile: shader_lib_filepath 
            error: &Error];
    id<MTLFunction> vertex_shader =
        [shader_library newFunctionWithName:
            @"vertex_shader"];
    id<MTLFunction> fragment_shader =
        [shader_library newFunctionWithName:
            @"fragment_shader"];
    
    if (Error != NULL)
    {
        [NSException
            raise: @"Can't Setup Metal" 
            format: @"Unable to shader libraries"];
    }
    
    // Setup combo pipeline that handles
    // both colored & textured triangles
    MTLRenderPipelineDescriptor * combo_pipeline_descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [combo_pipeline_descriptor
        setVertexFunction: vertex_shader];
    [combo_pipeline_descriptor
        setFragmentFunction: fragment_shader];
    // TODO: set pixel format
    combo_pipeline_descriptor
        .colorAttachments[0]
        .pixelFormat = pixel_format;
    
    _combo_pipeline_state =
        [metal_device
            newRenderPipelineStateWithDescriptor:
                combo_pipeline_descriptor 
            error:
                &Error];
    if (Error != NULL)
    {
        [NSException
            raise: @"Can't Setup Metal" 
            format: @"Unable to setup rendering pipeline state"];
    }
    
    uint32_t PageSize = getpagesize();
    uint32_t BufferedVertexSize = PageSize * 1000;
    
    // _render_commands = {};
    
    _vertex_buffers = [[NSMutableArray alloc] init];
    
    for (uint32_t frame_i = 0;
         frame_i < 3;
         frame_i++)
    {
        BufferedVertexCollection buffered_vertex = {};
        buffered_vertex.vertices =
            (Vertex *)mmap(
                0,
                BufferedVertexSize,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANON,
                -1,
                0);
        
        _render_commands.vertex_buffers[frame_i] =
            buffered_vertex;
        
        id<MTLBuffer> MetalBufferedVertex =
            [metal_device
                newBufferWithBytesNoCopy:
                    buffered_vertex.vertices
                length:
                    BufferedVertexSize
                options:
                    MTLResourceStorageModeShared
                deallocator:
                    nil];
        [_vertex_buffers
            addObject: MetalBufferedVertex];
    }
    
    _metal_textures = [[NSMutableArray alloc] init];

    // initialize a single texture array for
    // the global variable "bitmap"
    // MTLTextureDescriptor * bitmap_descriptor =
    //     [[MTLTextureDescriptor alloc] init];
    // bitmap_descriptor.textureType = MTLTexture2D;
    // bitmap_descriptor.pixelFormat =
    //     MTLPixelFormatRGBA8Unorm;
    // bitmap_descriptor.width = bitmap->image->width;
    // bitmap_descriptor.height = bitmap->image->height;
    // id<MTLTexture> bitmap_texture =
    //     [metal_device
    //         newTextureWithDescriptor:bitmap_descriptor];
    
    // initialize a texture array for each object
    // in the global var "texturearrays" 
    assert(TEXTUREARRAYS_SIZE > 0);
    for (uint32_t i = 0; i < TEXTUREARRAYS_SIZE; i++) {
        
        uint32_t slice_count =
            texture_arrays[i].sprite_rows *
                texture_arrays[i].sprite_columns;
        printf(
            "setting up texture %u with slice cnt: %u [%u,%u]\n",
            i,
            slice_count,
            texture_arrays[i].image->width,
            texture_arrays[i].image->height);
        
        MTLTextureDescriptor * texture_descriptor =
            [[MTLTextureDescriptor alloc] init]; 
        texture_descriptor.textureType = MTLTextureType2DArray;
        texture_descriptor.arrayLength = slice_count;
        texture_descriptor.pixelFormat =
            MTLPixelFormatRGBA8Unorm;
        texture_descriptor.width =
            texture_arrays[i].image->width
                / texture_arrays[i].sprite_columns;
        texture_descriptor.height =
            texture_arrays[i].image->height
                / texture_arrays[i].sprite_rows;
        
        id<MTLTexture> texture =
            [metal_device
                newTextureWithDescriptor:texture_descriptor];
        
        assert(texture_arrays[i].image->width >= 10);
        assert(4 * texture_arrays[i].image->width >= 40);
        uint32_t slice_i = 0;
        for (
            uint32_t row_i = 1;
            row_i <= texture_arrays[i].sprite_rows;
            row_i++)
        {
            for (
                uint32_t col_i = 1;
                col_i <= texture_arrays[i].sprite_columns;
                col_i++)
            {
                DecodedImage * new_slice =
                    extract_image(
                        /* texture_array: */ &texture_arrays[i],
                        /* x            : */ col_i,
                        /* y            : */ row_i);
                
                MTLRegion region = {
                    {
                        0,
                        0,
                        0
                    },
                    {
                        new_slice->width,
                        new_slice->height,
                        1
                    }
                };
                
                [texture
                    replaceRegion:
                        region
                    mipmapLevel:
                        0
                    slice:
                        slice_i
                    withBytes:
                        new_slice->rgba_values
                    bytesPerRow:
                        new_slice->width * 4
                    bytesPerImage:
                        /* docs: use 0 for anything other than
                           MTLTextureType3D textures */
                        0];
                
                // TODO: free heap memory
                // free(new_slice->rgba_values);
                // free(new_slice);
                slice_i++;
            }
        }
        
        [_metal_textures addObject: texture];
    }
    printf("finished setting up metal textures...\n");
    assert([_metal_textures count] > 0);
}

- (void)updateTextureArray: (int32_t)texturearray_i
    atSlice: (int32_t)texture_i
    withImg: (DecodedImage *)withImg
{
    MTLRegion region = {
        { 0, 0, 0 },
        { withImg->width, withImg->height, 1}};
    
    [_metal_textures[texturearray_i]
        replaceRegion:
            region
        mipmapLevel:
            0
        slice:
            texture_i
        withBytes:
            withImg->rgba_values
        bytesPerRow:
            withImg->width * 4
        bytesPerImage:
            /* docs: use 0 for anything other than
               MTLTextureType3D textures */
            0];
}

- (void)drawInMTKView:(MTKView *)view
{
    // TODO: this only works on retina
    // because on retina screens, the MTLViewport is 2x
    // the size of the window
    MTLViewport viewport = {
        0,
        0,
        window_width * 2.0f,
        window_height * 2.0f };
    
    uint32_t frame_i = _currentFrameIndex;
    
    Vertex * vertices_for_gpu =
        _render_commands.vertex_buffers[frame_i].vertices;
    uint32_t vertices_for_gpu_size = 0;
    
    software_render(
        /* next_gpu_workload: */
            vertices_for_gpu,
        /* next_gpu_workload_size: */
            &vertices_for_gpu_size);
    
    bitmap_render(
        /* next_gpu_workload: */
            vertices_for_gpu,
        /* next_gpu_workload_size: */
            &vertices_for_gpu_size);
    
    @autoreleasepool 
    {
        id<MTLCommandBuffer> command_buffer =
            [[self command_queue] commandBuffer];
        
        if (command_buffer == nil) {
            printf("error - failed to get command buffer¥n");
            return;
        }
        
        MTLRenderPassDescriptor *RenderPassDescriptor =
            [view currentRenderPassDescriptor];
        RenderPassDescriptor.colorAttachments[0].loadAction =
            MTLLoadActionClear;
        
        MTLClearColor clear_color =
            MTLClearColorMake(0.2f, 0.2f, 0.1f, 1.0f);
        RenderPassDescriptor.colorAttachments[0].clearColor =
            clear_color;
        
        id<MTLRenderCommandEncoder> render_encoder =
            [command_buffer
                renderCommandEncoderWithDescriptor:
                    RenderPassDescriptor];
        [render_encoder setViewport: viewport];
        
        // encode the drawing of all triangles 
        id<MTLBuffer> current_buffered_vertices =
            [[self vertex_buffers]
                objectAtIndex: _currentFrameIndex];
        [render_encoder
            setVertexBuffer: current_buffered_vertices  
            offset: 0 
            atIndex: 0];
        [render_encoder
            setRenderPipelineState:
                [self combo_pipeline_state]];
        
        for (
            uint32_t i = 0;
            i < TEXTUREARRAYS_SIZE;
            i++)
        {
            [render_encoder
                setFragmentTexture: _metal_textures[i]
                atIndex: i];
        }
        
        [render_encoder
            drawPrimitives: MTLPrimitiveTypeTriangle
            vertexStart: 0
            vertexCount: vertices_for_gpu_size];
        
        [render_encoder endEncoding];
        
        // Schedule a present once the framebuffer is complete
        // using the current drawable
        id<CAMetalDrawable> current_drawable =
            [view currentDrawable];
        [command_buffer presentDrawable: current_drawable];
        
        uint32_t next_index = _currentFrameIndex + 1;
        if (next_index > 2) { next_index = 0; }
        
        _currentFrameIndex = next_index;
        
        /* 
        [command_buffer
            addCompletedHandler:
                ^(id<MTLCommandBuffer> commandBuffer)
            {
            }];
        */
        
        [command_buffer commit];
    }
}

- (void)mtkView:(MTKView *)view
    drawableSizeWillChange:(CGSize)size
{

}

@end
