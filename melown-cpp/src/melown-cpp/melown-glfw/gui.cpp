#include <cstring>

#include <melown/map.hpp>
#include <melown/statistics.hpp>
#include <melown/options.hpp>
#include "mainWindow.hpp"
#include <nuklear.h>
#include <GLFW/glfw3.h>

class GuiImpl
{
public:
    struct vertex
    {
        float position[2];
        float uv[2];
        nk_byte col[4];
    };
    
    GuiImpl(MainWindow *window) : window(window),
        consumeEvents(true), prepareFirst(true),
        statTraversedDetails(false), statRenderedDetails(false)
    {
        { // load font
            struct nk_font_config cfg;
            memset(&cfg, 0, sizeof(cfg));
            cfg.oversample_h = 6;
            cfg.oversample_v = 6;
            nk_font_atlas_init_default(&atlas);
            nk_font_atlas_begin(&atlas);
            melown::Buffer buffer = melown::readInternalMemoryBuffer(
                        "data/fonts/roboto-regular.ttf");
            font = nk_font_atlas_add_from_memory(&atlas,
                buffer.data(), buffer.size(), 14, &cfg);
            melown::GpuTextureSpec spec;
            spec.verticalFlip = false;
            const void* img = nk_font_atlas_bake(&atlas,
                (int*)&spec.width, (int*)&spec.height, NK_FONT_ATLAS_RGBA32);
            spec.components = 4;
            spec.buffer.allocate(spec.width * spec.height * spec.components);
            memcpy(spec.buffer.data(), img, spec.buffer.size());
            fontTexture = std::make_shared<GpuTextureImpl>("font texture");
            fontTexture->loadTexture(spec);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            nk_font_atlas_end(&atlas, nk_handle_id(fontTexture->id), &null);
        }
        
        nk_init_default(&ctx, &font->handle);
        nk_buffer_init_default(&cmds);
        
        static const nk_draw_vertex_layout_element vertex_layout[] =
        {
            { NK_VERTEX_POSITION, NK_FORMAT_FLOAT, 0 },
            { NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, 8 },
            { NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, 16 },
            { NK_VERTEX_LAYOUT_END }
        };
        memset(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(vertex);
        config.vertex_alignment = alignof(vertex);
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = NK_ANTI_ALIASING_ON;
        config.line_AA = NK_ANTI_ALIASING_ON;
        config.null = null;
        
        { // load shader
            shader = std::make_shared<GpuShader>();
            melown::Buffer vert = melown::readInternalMemoryBuffer(
                        "data/shaders/gui.vert.glsl");
            melown::Buffer frag = melown::readInternalMemoryBuffer(
                        "data/shaders/gui.frag.glsl");
            shader->loadShaders(
                std::string(vert.data(), vert.size()),
                std::string(frag.data(), frag.size()));
        }
        
        { // prepare mesh buffers
            mesh = std::make_shared<GpuMeshImpl>("gui mesh");
            glGenVertexArrays(1, &mesh->vao);
            glGenBuffers(1, &mesh->vbo);
            glGenBuffers(1, &mesh->vio);
            glBindVertexArray(mesh->vao);
            glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->vio);
            glBufferData(GL_ARRAY_BUFFER, MaxVertexMemory,
                         NULL, GL_STREAM_DRAW);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, MaxElementMemory,
                         NULL, GL_STREAM_DRAW);
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                sizeof(vertex), (void*)0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 
                sizeof(vertex), (void*)8);
            glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                sizeof(vertex), (void*)16);
        }
    }
    
    ~GuiImpl()
    {
        nk_font_atlas_clear(&atlas);
        nk_free(&ctx);
    }
    
    void dispatch(int width, int height)
    {
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(mesh->vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->vio);
        shader->bind();
        
        { // proj matrix
            GLfloat ortho[4][4] = {
                    {2.0f, 0.0f, 0.0f, 0.0f},
                    {0.0f,-2.0f, 0.0f, 0.0f},
                    {0.0f, 0.0f,-1.0f, 0.0f},
                    {-1.0f,1.0f, 0.0f, 1.0f},
            };
            ortho[0][0] /= (GLfloat)width;
            ortho[1][1] /= (GLfloat)height;
            glUniformMatrix4fv(0, 1, GL_FALSE, &ortho[0][0]);
        }
        
        { // upload buffer data
            void *vertices = glMapBuffer(GL_ARRAY_BUFFER,
                                         GL_WRITE_ONLY);
            void *elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                         GL_WRITE_ONLY);
            nk_buffer vbuf, ebuf;
            nk_buffer_init_fixed(&vbuf, vertices, MaxVertexMemory);
            nk_buffer_init_fixed(&ebuf, elements, MaxElementMemory);
            nk_convert(&ctx, &cmds, &vbuf, &ebuf, &config);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        }
        
        { // draw commands
            struct nk_vec2 scale;
            scale.x = 1;
            scale.y = 1;
            const nk_draw_command *cmd;
            const nk_draw_index *offset = NULL;
            nk_draw_foreach(cmd, &ctx, &cmds)
            {
                if (!cmd->elem_count)
                    continue;
                glBindTexture(GL_TEXTURE_2D, cmd->texture.id);
                glScissor(
                    (GLint)(cmd->clip_rect.x * scale.x),
                    (GLint)((height - (GLint)(cmd->clip_rect.y
                                              + cmd->clip_rect.h)) * scale.y),
                    (GLint)(cmd->clip_rect.w * scale.x),
                    (GLint)(cmd->clip_rect.h * scale.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count,
                               GL_UNSIGNED_SHORT, offset);
                offset += cmd->elem_count;
            }
        }
        
        nk_clear(&ctx);
        
        glUseProgram(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glDisable(GL_SCISSOR_TEST);
    }
    
    void input()
    {
        consumeEvents = nk_item_is_any_active(&ctx);
        nk_input_begin(&ctx);
        glfwPollEvents();
        nk_input_key(&ctx, NK_KEY_DEL, glfwGetKey(window->window,
                GLFW_KEY_DELETE) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_ENTER, glfwGetKey(window->window,
                GLFW_KEY_ENTER) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_TAB, glfwGetKey(window->window,
                GLFW_KEY_TAB) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_BACKSPACE, glfwGetKey(window->window,
                GLFW_KEY_BACKSPACE) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_LEFT, glfwGetKey(window->window,
                GLFW_KEY_LEFT) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_RIGHT, glfwGetKey(window->window,
                GLFW_KEY_RIGHT) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_UP, glfwGetKey(window->window,
                GLFW_KEY_UP) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_DOWN, glfwGetKey(window->window,
                GLFW_KEY_DOWN) == GLFW_PRESS);
        if (glfwGetKey(window->window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window->window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
        {
            nk_input_key(&ctx, NK_KEY_COPY, glfwGetKey(window->window,
                    GLFW_KEY_C) == GLFW_PRESS);
            nk_input_key(&ctx, NK_KEY_PASTE, glfwGetKey(window->window,
                    GLFW_KEY_P) == GLFW_PRESS);
            nk_input_key(&ctx, NK_KEY_CUT, glfwGetKey(window->window,
                    GLFW_KEY_X) == GLFW_PRESS);
            nk_input_key(&ctx, NK_KEY_CUT, glfwGetKey(window->window,
                    GLFW_KEY_E) == GLFW_PRESS);
            nk_input_key(&ctx, NK_KEY_SHIFT, 1);
        } 
        else
        {
            nk_input_key(&ctx, NK_KEY_COPY, 0);
            nk_input_key(&ctx, NK_KEY_PASTE, 0);
            nk_input_key(&ctx, NK_KEY_CUT, 0);
            nk_input_key(&ctx, NK_KEY_SHIFT, 0);
        }
        double x, y;
        glfwGetCursorPos(window->window, &x, &y);
        nk_input_motion(&ctx, (int)x, (int)y);
        nk_input_button(&ctx, NK_BUTTON_LEFT, (int)x, (int)y,
                        glfwGetMouseButton(window->window,
                        GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        nk_input_button(&ctx, NK_BUTTON_MIDDLE, (int)x, (int)y,
                        glfwGetMouseButton(window->window,
                        GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
        nk_input_button(&ctx, NK_BUTTON_RIGHT, (int)x, (int)y,
                        glfwGetMouseButton(window->window,
                        GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
        nk_input_end(&ctx);
    }
    
    void mousePositionCallback(double xpos, double ypos)
    {
        if (!consumeEvents)
            window->mousePositionCallback(xpos, ypos);
    }

    void mouseScrollCallback(double xoffset, double yoffset)
    {
        struct nk_vec2 pos;
        pos.x = xoffset;
        pos.y = yoffset;
        nk_input_scroll(&ctx, pos);
        if (!consumeEvents)
            window->mouseScrollCallback(xoffset, yoffset);
    }

    void keyboardUnicodeCallback(unsigned int codepoint)
    {
        nk_input_unicode(&ctx, codepoint);
        if (!consumeEvents)
            window->keyboardUnicodeCallback(codepoint);
    }
    
    void prepareOptions()
    {
        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Options", nk_rect(10, 10, 250, 400), flags))
        {
            melown::MapOptions &o = window->map->options();
            float width = nk_window_get_content_region_size(&ctx).x - 15;
            float ratio[] = { width * 0.4, width * 0.5, width * 0.1 };
            nk_layout_row(&ctx, NK_STATIC, 16, 3, ratio);
            char buffer[256];
            // maxTexelToPixelScale
            nk_label(&ctx, "Detail control:", NK_TEXT_LEFT);
            o.maxTexelToPixelScale = nk_slide_float(&ctx,
                    1, o.maxTexelToPixelScale, 5, 0.01);
            sprintf(buffer, "%3.1f", o.maxTexelToPixelScale);
            nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            // maxConcurrentDownloads
            nk_label(&ctx, "Max downloads:", NK_TEXT_LEFT);
            o.maxConcurrentDownloads = nk_slide_int(&ctx,
                    1, o.maxConcurrentDownloads, 50, 1);
            sprintf(buffer, "%3d", o.maxConcurrentDownloads);
            nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            // navigation samples per view extent
            nk_label(&ctx, "Nav. samples:", NK_TEXT_LEFT);
            o.navigationSamplesPerViewExtent = nk_slide_int(&ctx,
                    1, o.navigationSamplesPerViewExtent, 16, 1);
            sprintf(buffer, "%3d", o.navigationSamplesPerViewExtent);
            nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            // render mesh wire boxes
            nk_label(&ctx, "Display:", NK_TEXT_LEFT);
            o.renderWireBoxes = nk_check_label(&ctx, "wireboxes",
                                               o.renderWireBoxes);
            nk_label(&ctx, "", NK_TEXT_LEFT);
            // render surrogates
            nk_label(&ctx, "", NK_TEXT_LEFT);
            o.renderSurrogates = nk_check_label(&ctx, "surrogates",
                                                o.renderSurrogates);
            nk_label(&ctx, "", NK_TEXT_LEFT);
            // render objective position
            nk_label(&ctx, "", NK_TEXT_LEFT);
            o.renderObjectPosition = nk_check_label(&ctx, "object. pos.",
                                                o.renderObjectPosition);
            nk_label(&ctx, "", NK_TEXT_LEFT);
        }
        nk_end(&ctx);
    }
    
    void prepareStatistics()
    {
        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Statistics", nk_rect(270, 10, 200, 400), flags))
        {
            melown::MapStatistics &s = window->map->statistics();
            float width = nk_window_get_content_region_size(&ctx).x - 15;
            float ratio[] = { width * 0.5, width * 0.5 };
            nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
            char buffer[256];
#define S(NAME, VAL, UNIT) { \
                nk_label(&ctx, NAME, NK_TEXT_LEFT); \
                sprintf(buffer, "%d" UNIT, VAL); \
                nk_label(&ctx, buffer, NK_TEXT_RIGHT); \
            }
            // general
            S("Frame:", s.frameIndex, "");
            S("Downloading:", s.currentDownloads, "");
            S("Gpu Memory:", s.currentGpuMemUse / 1024 / 1024, " MB");
            S("Nav. lod:", s.lastHeightRequestLod, "");
            nk_label(&ctx, "Z range:", NK_TEXT_LEFT);
            sprintf(buffer, "%0.0f - %0.0f", s.currentNearPlane,
                                            s.currentFarPlane);
            nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            // resources
            S("Res. active:", s.currentResources, "");
            S("Res. downloaded:", s.resourcesDownloaded, "");
            S("Res. read:", s.resourcesDiskLoaded, "");
            S("Res. gpu:", s.resourcesGpuLoaded, "");
            S("Res. released:", s.resourcesReleased, "");
            S("Res. ignored:", s.resourcesIgnored, "");
            S("Res. failed:", s.resourcesFailed, "");
            // traversed
            S("Traversed:", s.metaNodesTraversedTotal, "");
            nk_label(&ctx, "", NK_TEXT_LEFT);
            nk_checkbox_label(&ctx, "details", &statTraversedDetails);
            if (statTraversedDetails)
            {
                for (int i = 0; i < melown::MapStatistics::MaxLods; i++)
                {
                    if (s.metaNodesTraversedPerLod[i] == 0)
                        continue;
                    sprintf(buffer, "[%d]:", i);
                    S(buffer, s.metaNodesTraversedPerLod[i], "");
                }
            }
            // rendered
            S("Rendered:", s.meshesRenderedTotal, "");
            nk_label(&ctx, "", NK_TEXT_LEFT);
            nk_checkbox_label(&ctx, "details", &statRenderedDetails);
            if (statRenderedDetails)
            {
                for (int i = 0; i < melown::MapStatistics::MaxLods; i++)
                {
                    if (s.meshesRenderedPerLod[i] == 0)
                        continue;
                    sprintf(buffer, "[%d]:", i);
                    S(buffer, s.meshesRenderedPerLod[i], "");
                }
            }
#undef S
        }
        nk_end(&ctx);
    }
    
    void preparePosition()
    {
        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Position", nk_rect(480, 10, 200, 400), flags))
        {
            float width = nk_window_get_content_region_size(&ctx).x - 15;
            float ratio[] = { width * 0.4, width * 0.6 };
            nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
            char buffer[256];
            { // position
#define S(NAME, P, F) { \
                    nk_label(&ctx, NAME, NK_TEXT_LEFT); \
                    sprintf(buffer, F, (P).x); \
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT); \
                    nk_label(&ctx, "", NK_TEXT_LEFT); \
                    sprintf(buffer, F, (P).y); \
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT); \
                    nk_label(&ctx, "", NK_TEXT_LEFT); \
                    sprintf(buffer, F, (P).z); \
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT); \
                }
                melown::Point n = window->map->getPositionPoint();
                S("Pos. nav.:", n, "%.8f");
                melown::Point ph = window->map->convert(n,
                    melown::Srs::Navigation, melown::Srs::Physical);
                S("Pos. phys.:", ph, "%.5f");
                melown::Point pub = window->map->convert(n,
                    melown::Srs::Navigation, melown::Srs::Public);
                S("Pos. pub.:", pub, "%.8f");
            }
#undef S
            { // rotation
                melown::Point r = window->map->getPositionRotation();
                nk_label(&ctx, "Rotation:", NK_TEXT_LEFT);
                sprintf(buffer, "%5.1f", r.x);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "", NK_TEXT_LEFT);
                sprintf(buffer, "%5.1f", r.y);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "", NK_TEXT_LEFT);
                sprintf(buffer, "%5.1f", r.z);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "", NK_TEXT_LEFT);
                if (nk_button_label(&ctx, "Reset"))
                {
                    r.x = 0;
                    r.y = -90;
                    r.z = 0;
                    window->map->setPositionRotation(r);
                }
            }
            { // view extent
                nk_label(&ctx, "View extent:", NK_TEXT_LEFT);
                sprintf(buffer, "%10.1f", window->map->getPositionViewExtent());
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            }
            { // fov
                nk_label(&ctx, "Fov:", NK_TEXT_LEFT);
                sprintf(buffer, "%5.1f", window->map->getPositionFov());
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            }
        }
        nk_end(&ctx);
    }
    
    void prepareSurfaces()
    {
        // todo
    }
    
    void prepare(int width, int height)
    {
        prepareOptions();
        prepareStatistics();
        prepareSurfaces();
        preparePosition();
        prepareFirst = false;
    }
    
    void render(int width, int height)
    {
        prepare(width, height);
        dispatch(width, height);
    }
    
    nk_context ctx;
    nk_font_atlas atlas;
    nk_font *font;
    nk_buffer cmds;
    nk_convert_config config;
    nk_draw_null_texture null;
    bool consumeEvents;
    bool prepareFirst;
    
    int statTraversedDetails;
    int statRenderedDetails;
    
    MainWindow *window;
    std::shared_ptr<GpuTextureImpl> fontTexture;
    std::shared_ptr<GpuShader> shader;
    std::shared_ptr<GpuMeshImpl> mesh;
    
    static const int MaxVertexMemory = 512 * 1024;
    static const int MaxElementMemory = 128 * 1024;
};

void MainWindow::Gui::mousePositionCallback(double xpos, double ypos)
{
    impl->mousePositionCallback(xpos, ypos);
}

void MainWindow::Gui::mouseScrollCallback(double xoffset, double yoffset)
{
    impl->mouseScrollCallback(xoffset, yoffset);
}

void MainWindow::Gui::keyboardUnicodeCallback(unsigned int codepoint)
{
    impl->keyboardUnicodeCallback(codepoint);
}

void MainWindow::Gui::initialize(MainWindow *window)
{
    impl = std::make_shared<GuiImpl>(window);
}

void MainWindow::Gui::render(int width, int height)
{
    impl->render(width, height);
}

void MainWindow::Gui::input()
{
    impl->input();
    //glfwPollEvents();
}

void MainWindow::Gui::finalize()
{
    impl.reset();
}
