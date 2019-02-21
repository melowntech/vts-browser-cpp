/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstring>
#include <set>

#include <vts-browser/mapStatistics.hpp>
#include <vts-browser/cameraStatistics.hpp>
#include <vts-browser/cameraDraws.hpp>
#include <vts-browser/view.hpp>
#include <vts-browser/search.hpp>
#include <vts-browser/celestial.hpp>

#include <SDL2/SDL.h>

#include "mainWindow.hpp"
#include "guiSkin.hpp"
#include "editor.hpp"

using namespace vts;
using namespace renderer;

namespace
{

std::string controlOptionsPath()
{
    return "vts-browser-desktop.control-options.json";
}

const nk_rune FontUnicodeRanges[] = {
    // 0x0020, 0x007F, // Basic Latin
    // 0x00A0, 0x00FF, // Latin-1 Supplement
    // 0x0100, 0x017F, // Latin Extended-A
    // 0x0180, 0x024F, // Latin Extended-B
    // 0x0300, 0x036F, // Combining Diacritical Marks
    // 0x0400, 0x04FF, // Cyrillic
    0x0001, 0x5000, // all multilingual characters
    0
};

void clipBoardPaste(nk_handle, struct nk_text_edit *edit)
{
    const char *text = SDL_GetClipboardText();
    if (text)
        nk_textedit_paste(edit, text, strlen(text));
}

void clipBoardCopy(nk_handle, const char *text, int len)
{
    assert(len < 300);
    char buffer[301];
    memcpy(buffer, text, len);
    buffer[len] = 0;
    SDL_SetClipboardText(buffer);
}

static const char *traverseModeNames[] = {
    "None",
    "Flat",
    "Stable",
    "Balanced",
    "Hierarchical",
    "Fixed",
};

static const char *navigationTypeNames[] = {
    "Instant",
    "Quick",
    "FlyOver",
};

static const char *navigationModeNames[] = {
    "Azimuthal",
    "Free",
    "Dynamic",
    "Seamless",
};

static const char *lodBlendingModeNames[] = {
    "Off",
    "Basic",
    "Precise",
};

} // namespace

Mark::Mark() : open(false)
{}

class GuiImpl
{
public:
    struct vertex
    {
        float position[2];
        float uv[2];
        nk_byte col[4];
    };

    GuiImpl(MainWindow *window) :
        posAutoMotion(0,0,0), posAutoRotation(0),
        viewExtentLimitScaleMin(0),
        viewExtentLimitScaleMax(std::numeric_limits<double>::infinity()),
        positionSrs(2), window(window), prepareFirst(true), hideTheGui(false)
    {
        gladLoadGLLoader(&SDL_GL_GetProcAddress);

        searchText[0] = 0;
        searchTextPrev[0] = 0;
        positionInputText[0] = 0;

        // load font
        {
            struct nk_font_config cfg = nk_font_config(0);
            cfg.oversample_h = 3;
            cfg.oversample_v = 2;
            cfg.range = FontUnicodeRanges;
            nk_font_atlas_init_default(&atlas);
            nk_font_atlas_begin(&atlas);
            Buffer buffer = readInternalMemoryBuffer(
                        "data/fonts/Roboto-Regular.ttf");
            font = nk_font_atlas_add_from_memory(&atlas,
                buffer.data(), buffer.size(), 14, &cfg);
            GpuTextureSpec spec;
            const void* img = nk_font_atlas_bake(&atlas,
                (int*)&spec.width, (int*)&spec.height, NK_FONT_ATLAS_RGBA32);
            spec.components = 4;
            spec.buffer.allocate(spec.width * spec.height * spec.components);
            memcpy(spec.buffer.data(), img, spec.buffer.size());
            fontTexture = std::make_shared<Texture>();
            fontTexture->load(spec);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            nk_font_atlas_end(&atlas, nk_handle_id(fontTexture->getId()),
                              &null);
        }

        nk_init_default(&ctx, &font->handle);
        nk_buffer_init_default(&cmds);

        ctx.clip.paste = &clipBoardPaste;
        ctx.clip.copy = &clipBoardCopy;
        ctx.clip.userdata.ptr = window->window;

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

        initializeGuiSkin(ctx, skinMedia, skinTexture);

        // load shader
        {
            shader = std::make_shared<Shader>();
            Buffer vert = readInternalMemoryBuffer(
                        "data/shaders/gui.vert.glsl");
            Buffer frag = readInternalMemoryBuffer(
                        "data/shaders/gui.frag.glsl");
            shader->load(
                std::string(vert.data(), vert.size()),
                std::string(frag.data(), frag.size()));
            std::vector<uint32> &uls = shader->uniformLocations;
            GLuint id = shader->getId();
            uls.push_back(glGetUniformLocation(id, "ProjMtx"));
            glUseProgram(id);
            glUniform1i(glGetUniformLocation(id, "Texture"), 0);
        }

        // prepare mesh buffers
        {
            GLuint vao = 0, vbo = 0, vio = 0;
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &vio);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vio);
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
            mesh = std::make_shared<Mesh>();
            mesh->load(vao, vbo, vio);
        }

        // load control options
        try
        {
            window->navigation->options().applyJson(
                        readLocalFileBuffer(controlOptionsPath()).str());
        }
        catch(...)
        {
            // do nothing
        }
    }

    ~GuiImpl()
    {
        nk_buffer_free(&cmds);
        nk_font_atlas_clear(&atlas);
        nk_free(&ctx);
    }

    void dispatch(int width, int height)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(mesh->getVao());
        glBindBuffer(GL_ARRAY_BUFFER, mesh->getVbo());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->getVio());
        shader->bind();

        // proj matrix
        {
            GLfloat ortho[4][4] = {
                    {2.0f, 0.0f, 0.0f, 0.0f},
                    {0.0f,-2.0f, 0.0f, 0.0f},
                    {0.0f, 0.0f,-1.0f, 0.0f},
                    {-1.0f,1.0f, 0.0f, 1.0f},
            };
            ortho[0][0] /= (GLfloat)width;
            ortho[1][1] /= (GLfloat)height;
            glUniformMatrix4fv(shader->uniformLocations[0], 1,
                    GL_FALSE, &ortho[0][0]);
        }

        // upload buffer data
        {
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

        // draw commands
        {
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
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisable(GL_SCISSOR_TEST);
    }

    int handleEvent(SDL_Event *evt)
    {
        // hide the gui toggle
        const Uint8* state = SDL_GetKeyboardState(0);
        if (evt->type == SDL_KEYUP && state[SDL_SCANCODE_LCTRL] && evt->key.keysym.sym == SDLK_g)
        {
            hideTheGui = !hideTheGui;
            return 1;
        }

        // some code copied from the demos
        if (evt->type == SDL_KEYUP || evt->type == SDL_KEYDOWN) {
            /* key events */
            int down = evt->type == SDL_KEYDOWN;
            SDL_Keycode sym = evt->key.keysym.sym;
            if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
                nk_input_key(&ctx, NK_KEY_SHIFT, down);
            else if (sym == SDLK_DELETE)
                nk_input_key(&ctx, NK_KEY_DEL, down);
            else if (sym == SDLK_RETURN)
                nk_input_key(&ctx, NK_KEY_ENTER, down);
            else if (sym == SDLK_TAB)
                nk_input_key(&ctx, NK_KEY_TAB, down);
            else if (sym == SDLK_BACKSPACE)
                nk_input_key(&ctx, NK_KEY_BACKSPACE, down);
            else if (sym == SDLK_HOME) {
                nk_input_key(&ctx, NK_KEY_TEXT_START, down);
                nk_input_key(&ctx, NK_KEY_SCROLL_START, down);
            } else if (sym == SDLK_END) {
                nk_input_key(&ctx, NK_KEY_TEXT_END, down);
                nk_input_key(&ctx, NK_KEY_SCROLL_END, down);
            } else if (sym == SDLK_PAGEDOWN) {
                nk_input_key(&ctx, NK_KEY_SCROLL_DOWN, down);
            } else if (sym == SDLK_PAGEUP) {
                nk_input_key(&ctx, NK_KEY_SCROLL_UP, down);
            } else if (sym == SDLK_z)
                nk_input_key(&ctx, NK_KEY_TEXT_UNDO, down && state[SDL_SCANCODE_LCTRL]);
            else if (sym == SDLK_r)
                nk_input_key(&ctx, NK_KEY_TEXT_REDO, down && state[SDL_SCANCODE_LCTRL]);
            else if (sym == SDLK_c)
                nk_input_key(&ctx, NK_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
            else if (sym == SDLK_v)
                nk_input_key(&ctx, NK_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
            else if (sym == SDLK_x)
                nk_input_key(&ctx, NK_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
            else if (sym == SDLK_b)
                nk_input_key(&ctx, NK_KEY_TEXT_LINE_START, down && state[SDL_SCANCODE_LCTRL]);
            else if (sym == SDLK_e)
                nk_input_key(&ctx, NK_KEY_TEXT_LINE_END, down && state[SDL_SCANCODE_LCTRL]);
            else if (sym == SDLK_UP)
                nk_input_key(&ctx, NK_KEY_UP, down);
            else if (sym == SDLK_DOWN)
                nk_input_key(&ctx, NK_KEY_DOWN, down);
            else if (sym == SDLK_LEFT) {
                if (state[SDL_SCANCODE_LCTRL])
                    nk_input_key(&ctx, NK_KEY_TEXT_WORD_LEFT, down);
                else nk_input_key(&ctx, NK_KEY_LEFT, down);
            } else if (sym == SDLK_RIGHT) {
                if (state[SDL_SCANCODE_LCTRL])
                    nk_input_key(&ctx, NK_KEY_TEXT_WORD_RIGHT, down);
                else nk_input_key(&ctx, NK_KEY_RIGHT, down);
            } else return 0;
            return 1;
        } else if (evt->type == SDL_MOUSEBUTTONDOWN || evt->type == SDL_MOUSEBUTTONUP) {
            /* mouse button */
            int down = evt->type == SDL_MOUSEBUTTONDOWN;
            const int x = evt->button.x, y = evt->button.y;
            if (evt->button.button == SDL_BUTTON_LEFT) {
                if (evt->button.clicks > 1)
                    nk_input_button(&ctx, NK_BUTTON_DOUBLE, x, y, down);
                nk_input_button(&ctx, NK_BUTTON_LEFT, x, y, down);
            } else if (evt->button.button == SDL_BUTTON_MIDDLE)
                nk_input_button(&ctx, NK_BUTTON_MIDDLE, x, y, down);
            else if (evt->button.button == SDL_BUTTON_RIGHT)
                nk_input_button(&ctx, NK_BUTTON_RIGHT, x, y, down);
            return 1;
        } else if (evt->type == SDL_MOUSEMOTION) {
            /* mouse motion */
            if (ctx.input.mouse.grabbed) {
                int x = (int)ctx.input.mouse.prev.x, y = (int)ctx.input.mouse.prev.y;
                nk_input_motion(&ctx, x + evt->motion.xrel, y + evt->motion.yrel);
            } else nk_input_motion(&ctx, evt->motion.x, evt->motion.y);
            return 1;
        } else if (evt->type == SDL_TEXTINPUT) {
            /* text input */
            nk_glyph glyph;
            memcpy(glyph, evt->text.text, NK_UTF_SIZE);
            nk_input_glyph(&ctx, glyph);
            return 1;
        } else if (evt->type == SDL_MOUSEWHEEL) {
            /* mouse wheel */
            nk_input_scroll(&ctx,nk_vec2((float)evt->wheel.x,(float)evt->wheel.y));
            return 1;
        }
        return 0;
    }

    void prepareOptions()
    {
        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Options", nk_rect(10, 10, 250, 600), flags))
        {
            MapRuntimeOptions &mr = window->map->options();
            CameraOptions &c = window->camera->options();
            NavigationOptions &n = window->navigation->options();
            AppOptions &a = window->appOptions;
            renderer::RenderOptions &r = window->render.options();
            float width = nk_window_get_content_region_size(&ctx).x - 30;
            char buffer[256];

            // camera control sensitivity
            if (nk_tree_push(&ctx, NK_TREE_TAB, "Mouse Sensitivity",
                NK_MINIMIZED))
            {
                float ratio[] = { width * 0.4f, width * 0.45f,
                    width * 0.15f };
                nk_layout_row(&ctx, NK_STATIC, 16, 3, ratio);

                // sensitivity
                nk_label(&ctx, "Pan speed:", NK_TEXT_LEFT);
                n.sensitivityPan = nk_slide_float(&ctx,
                        0.1, n.sensitivityPan, 3, 0.01);
                sprintf(buffer, "%4.2f", n.sensitivityPan);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Zoom speed:", NK_TEXT_LEFT);
                n.sensitivityZoom = nk_slide_float(&ctx,
                        0.1, n.sensitivityZoom, 3, 0.01);
                sprintf(buffer, "%4.2f", n.sensitivityZoom);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Rotate speed:", NK_TEXT_LEFT);
                n.sensitivityRotate = nk_slide_float(&ctx,
                        0.1, n.sensitivityRotate, 3, 0.01);
                sprintf(buffer, "%4.2f", n.sensitivityRotate);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);

                // inertia
                nk_label(&ctx, "Pan inertia:", NK_TEXT_LEFT);
                n.inertiaPan = nk_slide_float(&ctx,
                        0, n.inertiaPan, 0.99, 0.01);
                sprintf(buffer, "%4.2f", n.inertiaPan);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Zoom inertia:", NK_TEXT_LEFT);
                n.inertiaZoom = nk_slide_float(&ctx,
                        0, n.inertiaZoom, 0.99, 0.01);
                sprintf(buffer, "%4.2f", n.inertiaZoom);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Rotate inertia:", NK_TEXT_LEFT);
                n.inertiaRotate = nk_slide_float(&ctx,
                        0, n.inertiaRotate, 0.99, 0.01);
                sprintf(buffer, "%4.2f", n.inertiaRotate);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);

                nk_layout_row(&ctx, NK_STATIC, 16, 1, &width);

                // save
                if (nk_button_label(&ctx, "Save"))
                {
                    try
                    {
                        writeLocalFileBuffer(controlOptionsPath(),
                            Buffer(n.toJson()));
                    }
                    catch(...)
                    {
                        // do nothing
                    }
                }

                // load
                if (nk_button_label(&ctx, "Load"))
                {
                    try
                    {
                        n.applyJson(readLocalFileBuffer(
                            controlOptionsPath()).str());
                    }
                    catch(...)
                    {
                        // do nothing
                    }
                }

                // reset
                if (nk_button_label(&ctx, "Reset"))
                {
                    NavigationOptions d;
                    n.sensitivityPan       = d.sensitivityPan;
                    n.sensitivityZoom      = d.sensitivityZoom;
                    n.sensitivityRotate    = d.sensitivityRotate;
                    n.inertiaPan           = d.inertiaPan;
                    n.inertiaZoom          = d.inertiaZoom;
                    n.inertiaRotate        = d.inertiaRotate;
                }

                // end group
                nk_tree_pop(&ctx);
            }

            // navigation
            if (nk_tree_push(&ctx, NK_TREE_TAB, "Navigation", NK_MINIMIZED))
            {
                {
                    float ratio[] = { width * 0.4f, width * 0.6f };
                    nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);

                    // navigation type
                    nk_label(&ctx, "Nav. type:", NK_TEXT_LEFT);
                    if (nk_combo_begin_label(&ctx,
                                 navigationTypeNames[(int)n.navigationType],
                                 nk_vec2(nk_widget_width(&ctx), 200)))
                    {
                        nk_layout_row_dynamic(&ctx, 16, 1);
                        for (unsigned i = 0; i < sizeof(navigationTypeNames)
                             / sizeof(navigationTypeNames[0]); i++)
                        {
                            if (nk_combo_item_label(&ctx,
                                    navigationTypeNames[i], NK_TEXT_LEFT))
                                n.navigationType = (NavigationType)i;
                        }
                        nk_combo_end(&ctx);
                    }

                    // navigation mode
                    nk_label(&ctx, "Nav. mode:", NK_TEXT_LEFT);
                    if (nk_combo_begin_label(&ctx,
                                 navigationModeNames[(int)n.navigationMode],
                                 nk_vec2(nk_widget_width(&ctx), 200)))
                    {
                        nk_layout_row_dynamic(&ctx, 16, 1);
                        for (unsigned i = 0; i < sizeof(navigationModeNames)
                             / sizeof(navigationModeNames[0]); i++)
                        {
                            if (nk_combo_item_label(&ctx,
                                        navigationModeNames[i],NK_TEXT_LEFT))
                                n.navigationMode = (NavigationMode)i;
                        }
                        nk_combo_end(&ctx);
                    }
                }

                {
                    float ratio[] = { width * 0.4f, width * 0.45f,
                                      width * 0.15f };
                    nk_layout_row(&ctx, NK_STATIC, 16, 3, ratio);

                    // navigation max view extent multiplier
                    nk_label(&ctx, "Piha zoom:", NK_TEXT_LEFT);
                    n.navigationPihaViewExtentMult = nk_slide_float(&ctx,
                            1.002, n.navigationPihaViewExtentMult, 1.2, 0.002);
                    sprintf(buffer, "%5.3f", n.navigationPihaViewExtentMult);
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT);

                    // navigation max position change
                    nk_label(&ctx, "Piha move:", NK_TEXT_LEFT);
                    n.navigationPihaPositionChange = nk_slide_float(&ctx,
                            0.002, n.navigationPihaPositionChange, 0.2, 0.002);
                    sprintf(buffer, "%5.3f", n.navigationPihaPositionChange);
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT);

                    // navigation samples per view extent
                    nk_label(&ctx, "Nav. samples:", NK_TEXT_LEFT);
                    n.navigationSamplesPerViewExtent = nk_slide_int(&ctx,
                            1, n.navigationSamplesPerViewExtent, 16, 1);
                    sprintf(buffer, "%3d", n.navigationSamplesPerViewExtent);
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT);

                    // altitude fade out
                    nk_label(&ctx, "Altitude fade:", NK_TEXT_LEFT);
                    n.cameraAltitudeFadeOutFactor = nk_slide_float(&ctx,
                            0, n.cameraAltitudeFadeOutFactor, 1, 0.01);
                    sprintf(buffer, "%4.2f", n.cameraAltitudeFadeOutFactor);
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                }

                // end group
                nk_tree_pop(&ctx);
            }

            // rendering
            if (nk_tree_push(&ctx, NK_TREE_TAB, "Rendering", NK_MINIMIZED))
            {
                {
                    float ratio[] = { width * 0.4f, width * 0.45f,
                        width * 0.15f };
                    nk_layout_row(&ctx, NK_STATIC, 16, 3, ratio);

                    // traverse mode (surfaces)
                    nk_label(&ctx, "Surfaces:", NK_TEXT_LEFT);
                    if (nk_combo_begin_label(&ctx,
                             traverseModeNames[(int)c.traverseModeSurfaces],
                             nk_vec2(nk_widget_width(&ctx), 200)))
                    {
                        nk_layout_row_dynamic(&ctx, 16, 1);
                        for (unsigned i = 0; i < sizeof(traverseModeNames)
                             / sizeof(traverseModeNames[0]); i++)
                        {
                            if (nk_combo_item_label(&ctx,
                                    traverseModeNames[i],
                                    NK_TEXT_LEFT))
                                c.traverseModeSurfaces = (TraverseMode)i;
                        }
                        nk_combo_end(&ctx);
                    }
                    nk_label(&ctx, "", NK_TEXT_RIGHT);

                    // traverse mode (geodata)
                    nk_label(&ctx, "Geodata:", NK_TEXT_LEFT);
                    if (nk_combo_begin_label(&ctx,
                             traverseModeNames[(int)c.traverseModeGeodata],
                             nk_vec2(nk_widget_width(&ctx), 200)))
                    {
                        nk_layout_row_dynamic(&ctx, 16, 1);
                        for (unsigned i = 0; i < sizeof(traverseModeNames)
                             / sizeof(traverseModeNames[0]); i++)
                        {
                            if (nk_combo_item_label(&ctx,
                                               traverseModeNames[i],
                                               NK_TEXT_LEFT))
                                c.traverseModeGeodata = (TraverseMode)i;
                        }
                        nk_combo_end(&ctx);
                    }
                    nk_label(&ctx, "", NK_TEXT_RIGHT);

                    // balanced grids
                    if (c.traverseModeSurfaces == TraverseMode::Balanced
                        || c.traverseModeGeodata == TraverseMode::Balanced)
                    {
                        // balancedGridLodOffset
                        nk_label(&ctx, "Grid offset:", NK_TEXT_LEFT);
                        c.balancedGridLodOffset = nk_slide_int(&ctx,
                            -1, c.balancedGridLodOffset, 10, 1);
                        sprintf(buffer, "%d", c.balancedGridLodOffset);
                        nk_label(&ctx, buffer, NK_TEXT_RIGHT);

                        // balancedGridNeighborsDistance
                        nk_label(&ctx, "Grid neighbors:", NK_TEXT_LEFT);
                        c.balancedGridNeighborsDistance = nk_slide_int(&ctx,
                            0, c.balancedGridNeighborsDistance, 3, 1);
                        sprintf(buffer, "%d", c.balancedGridNeighborsDistance);
                        nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                    }

                    // fixed traversal
                    if (c.traverseModeSurfaces == TraverseMode::Fixed
                        || c.traverseModeGeodata == TraverseMode::Fixed)
                    {
                        // fixedTraversalLod
                        nk_label(&ctx, "Fixed Lod:", NK_TEXT_LEFT);
                        c.fixedTraversalLod = nk_slide_int(&ctx,
                            0, c.fixedTraversalLod, 30, 1);
                        sprintf(buffer, "%d", c.fixedTraversalLod);
                        nk_label(&ctx, buffer, NK_TEXT_RIGHT);

                        // fixedTraversalDistance
                        nk_label(&ctx, "Fixed Distance:", NK_TEXT_LEFT);
                        c.fixedTraversalDistance = nk_slide_float(&ctx,
                            100, c.fixedTraversalDistance, 10000, 100);
                        sprintf(buffer, "%5.0f", c.fixedTraversalDistance);
                        nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                    }

                    // lodBlending
                    nk_label(&ctx, "Lod blending:", NK_TEXT_LEFT);
                    if (nk_combo_begin_label(&ctx,
                        lodBlendingModeNames[c.lodBlending],
                        nk_vec2(nk_widget_width(&ctx), 200)))
                    {
                        nk_layout_row_dynamic(&ctx, 16, 1);
                        for (unsigned i = 0; i < sizeof(lodBlendingModeNames)
                            / sizeof(lodBlendingModeNames[0]); i++)
                        {
                            if (nk_combo_item_label(&ctx,
                                lodBlendingModeNames[i],
                                NK_TEXT_LEFT))
                                c.lodBlending = i;
                        }
                        nk_combo_end(&ctx);
                    }
                    nk_label(&ctx, "", NK_TEXT_RIGHT);

                    // lodBlendingDuration
                    if (c.lodBlending)
                    {
                        nk_label(&ctx, "Blend duration:", NK_TEXT_LEFT);
                        c.lodBlendingDuration = nk_slide_int(&ctx,
                            0, c.lodBlendingDuration, 1000, 5);
                        sprintf(buffer, "%4d", c.lodBlendingDuration);
                        nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                    }

                    // cullingOffsetDistance
                    nk_label(&ctx, "Culling offset:", NK_TEXT_LEFT);
                    c.cullingOffsetDistance = nk_slide_float(&ctx,
                        0.0, c.cullingOffsetDistance, 500, 1.0);
                    sprintf(buffer, "%3.1f", c.cullingOffsetDistance);
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT);

                    // maxTexelToPixelScale
                    nk_label(&ctx, "Texel to pixel:", NK_TEXT_LEFT);
                    c.maxTexelToPixelScale = nk_slide_float(&ctx,
                            0.3, c.maxTexelToPixelScale, 30, 0.1);
                    sprintf(buffer, "%3.1f", c.maxTexelToPixelScale);
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT);

                    // antialiasing samples
                    nk_label(&ctx, "Antialiasing:", NK_TEXT_LEFT);
                    r.antialiasingSamples = nk_slide_int(&ctx,
                            1, r.antialiasingSamples, 16, 1);
                    if (r.antialiasingSamples > 1)
                    {
                        sprintf(buffer, "%d", r.antialiasingSamples);
                        nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                    }
                    else
                        nk_label(&ctx, "no", NK_TEXT_RIGHT);

                    // maxResourcesMemory
                    nk_label(&ctx, "Target memory:", NK_TEXT_LEFT);
                    mr.targetResourcesMemoryKB = 1024 * nk_slide_int(&ctx,
                            0, mr.targetResourcesMemoryKB / 1024, 2048, 32);
                    sprintf(buffer, "%3d", mr.targetResourcesMemoryKB / 1024);
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT);

                    // text scale
                    {
                        float &s = window->render.options().textScale;
                        nk_label(&ctx, "Text Scale:", NK_TEXT_LEFT);
                        s = nk_slide_float(&ctx, 0.2, s, 5, 0.1);
                        sprintf(buffer, "%3.1f", s);
                        nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                    }
                }

                // end group
                nk_tree_pop(&ctx);
            }

            // display
            if (nk_tree_push(&ctx, NK_TREE_TAB, "Display", NK_MINIMIZED))
            {
                nk_layout_row(&ctx, NK_STATIC, 16, 1, &width);

                // render atmosphere
                r.renderAtmosphere = nk_check_label(&ctx, "atmosphere",
                                                   r.renderAtmosphere);

                // render mesh boxes
                c.debugRenderMeshBoxes = nk_check_label(&ctx, "mesh boxes",
                                                   c.debugRenderMeshBoxes);

                // render tile boxes
                c.debugRenderTileBoxes = nk_check_label(&ctx, "tile boxes",
                                                    c.debugRenderTileBoxes);

                // render subtile boxes
                c.debugRenderSubtileBoxes = nk_check_label(&ctx,
                    "subtile boxes", c.debugRenderSubtileBoxes);

                // render surrogates
                c.debugRenderSurrogates = nk_check_label(&ctx, "surrogates",
                                                    c.debugRenderSurrogates);

                // render objective position
                n.debugRenderObjectPosition = nk_check_label(&ctx,
                            "objective pos.", n.debugRenderObjectPosition);

                // render target position
                n.debugRenderTargetPosition = nk_check_label(&ctx,
                            "target pos.", n.debugRenderTargetPosition);

                // flat shading
                c.debugFlatShading = nk_check_label(&ctx, "flat shading",
                                                    c.debugFlatShading);

                // polygon edges
                r.renderPolygonEdges = nk_check_label(&ctx, "edges",
                                                r.renderPolygonEdges);

                // render compas
                nk_checkbox_label(&ctx,
                    "compas", &a.renderCompas);

                // end group
                nk_tree_pop(&ctx);
            }

            // debug
            if (nk_tree_push(&ctx, NK_TREE_TAB, "Debug", NK_MINIMIZED))
            {
                nk_layout_row(&ctx, NK_STATIC, 16, 1, &width);

                // enable camera normalization
                n.cameraNormalization = nk_check_label(&ctx,
                                "camera normalization",
                                n.cameraNormalization);

                // camera zoom limit
                {
                    int e = viewExtentLimitScaleMax
                            == std::numeric_limits<double>::infinity();
                    int ePrev = e;
                    nk_checkbox_label(&ctx, "zoom limit", &e);
                    if (e != ePrev)
                    {
                        std::swap(viewExtentLimitScaleMin,
                                  n.viewExtentLimitScaleMin);
                        std::swap(viewExtentLimitScaleMax,
                                  n.viewExtentLimitScaleMax);
                    }
                }

                // detached camera
                c.debugDetachedCamera = nk_check_label(&ctx,
                                "detached camera", c.debugDetachedCamera);

                // virtual surfaces
                {
                    bool old = mr.debugVirtualSurfaces;
                    mr.debugVirtualSurfaces = nk_check_label(&ctx,
                            "virtual surfaces",
                            mr.debugVirtualSurfaces);
                    if (old != mr.debugVirtualSurfaces)
                        window->map->purgeViewCache();
                }

                // coarseness disks
                mr.debugCoarsenessDisks = nk_check_label(&ctx,
                    "coarseness disks", mr.debugCoarsenessDisks);

                // geodata validation
                {
                    bool old = mr.debugValidateGeodataStyles;
                    mr.debugValidateGeodataStyles = nk_check_label(&ctx,
                            "validate geodata styles",
                            mr.debugValidateGeodataStyles);
                    if (old != mr.debugValidateGeodataStyles)
                        window->map->purgeViewCache();
                }

                // purge disk cache
                if (nk_button_label(&ctx, "Purge disk cache"))
                    window->map->purgeDiskCache();

                // end group
                nk_tree_pop(&ctx);
            }
        }

        // end window
        nk_end(&ctx);
    }

    template<class T>
    void S(const char *name, T value, const char *unit)
    {
        nk_label(&ctx, name, NK_TEXT_LEFT);
        std::ostringstream ss;
        ss << value;
        if (unit && unit[0] != 0)
            ss << unit;
        nk_label(&ctx, ss.str().c_str(), NK_TEXT_RIGHT);
    }

    void prepareStatistics()
    {
        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Statistics", nk_rect(270, 10, 250, 650), flags))
        {
            MapStatistics &ms = window->map->statistics();
            CameraStatistics &cs = window->camera->statistics();
            float width = nk_window_get_content_region_size(&ctx).x - 30;

            char buffer[256];

            // general
            if (nk_tree_push(&ctx, NK_TREE_TAB, "General", NK_MAXIMIZED))
            {
                float ratio[] = { width * 0.5f, width * 0.5f };
                nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);

                nk_label(&ctx, "Loading:", NK_TEXT_LEFT);
                nk_prog(&ctx, (int)(
                        1000 * window->map->getMapRenderProgress()),
                        1000, false);

                //S("Time map min:", window->timingMapSmooth.min(), " ms");
                S("Time map avg:", window->timingMapSmooth.avg(), " ms");
                S("Time map max:", window->timingMapSmooth.max(), " ms");
                S("Time app:", window->timingAppProcess, " ms");
                //S("Time frame min:", window->timingFrameSmooth.min(), " ms");
                S("Time frame avg:", window->timingFrameSmooth.avg(), " ms");
                S("Time frame max:", window->timingFrameSmooth.max(), " ms");
                S("Render tick:", ms.renderTicks, "");

                nk_tree_pop(&ctx);
            }

            // resources
            if (nk_tree_push(&ctx, NK_TREE_TAB, "Resources", NK_MAXIMIZED))
            {
                float ratio[] = { width * 0.5f, width * 0.5f };
                nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);

                S("GPU memory:", ms.currentGpuMemUseKB / 1024, " MB");
                S("RAM memory:", ms.currentRamMemUseKB / 1024, " MB");
                S("Node meta updates:", cs.currentNodeMetaUpdates, "");
                S("Node draw updates:", cs.currentNodeDrawsUpdates, "");
                S("Preparing:", ms.resourcesPreparing, "");
                S("Downloading:", ms.resourcesDownloading, "");
                S("Downloaded:", ms.resourcesDownloaded, "");
                S("Disk loaded:", ms.resourcesDiskLoaded, "");
                S("Active:", ms.resourcesActive, "");
                S("Processed:", ms.resourcesProcessed, "");
                S("Created:", ms.resourcesCreated, "");
                S("Released:", ms.resourcesReleased, "");
                S("Failed:", ms.resourcesFailed, "");

                nk_tree_pop(&ctx);
            }

            // traversed
            if (nk_tree_push(&ctx, NK_TREE_TAB, "Traversed nodes",
                             NK_MINIMIZED))
            {
                float ratio[] = { width * 0.5f, width * 0.5f };
                nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);

                for (unsigned i = 0; i < CameraStatistics::MaxLods; i++)
                {
                    if (cs.metaNodesTraversedPerLod[i] == 0)
                        continue;
                    sprintf(buffer, "[%d]:", i);
                    S(buffer, cs.metaNodesTraversedPerLod[i], "");
                }

                S("Total:", cs.metaNodesTraversedTotal, "");
                S("Grid nodes:", cs.currentGridNodes, "");

                nk_tree_pop(&ctx);
            }

            // rendered nodes
            if (nk_tree_push(&ctx, NK_TREE_TAB, "Rendered nodes",
                             NK_MINIMIZED))
            {
                float ratio[] = { width * 0.5f, width * 0.5f };
                nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);

                for (unsigned i = 0; i < CameraStatistics::MaxLods; i++)
                {
                    if (cs.nodesRenderedPerLod[i] == 0)
                        continue;
                    sprintf(buffer, "[%d]:", i);
                    S(buffer, cs.nodesRenderedPerLod[i], "");
                }
                S("Total:", cs.nodesRenderedTotal, "");

                nk_tree_pop(&ctx);
            }

            // draw calls
            if (nk_tree_push(&ctx, NK_TREE_TAB, "Draw calls",
                             NK_MINIMIZED))
            {
                float ratio[] = { width * 0.5f, width * 0.5f };
                nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);

                const CameraDraws &d = window->camera->draws();
                S("Opaque: ", d.opaque.size(), "");
                S("Transparent: ", d.transparent.size(), "");
                S("Geodata: ", d.geodata.size(), "");
                S("Infographics: ", d.infographics.size(), "");

                nk_tree_pop(&ctx);
            }
        }

        // end window
        nk_end(&ctx);
    }

    void preparePosition()
    {
        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Position", nk_rect(890, 10, 250, 400), flags))
        {
            float width = nk_window_get_content_region_size(&ctx).x - 30;

            // loading?
            if (!window->map->getMapconfigAvailable())
            {
                nk_layout_row(&ctx, NK_STATIC, 16, 1, &width);
                nk_label(&ctx, "Loading...", NK_TEXT_LEFT);
                nk_end(&ctx);
                return;
            }

            float ratio[] = { width * 0.4f, width * 0.6f };
            nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
            char buffer[256];

            // input
            {
                nk_label(&ctx, "Input:", NK_TEXT_LEFT);
                if (nk_button_label(&ctx, "Use from clipboard"))
                {
                    try
                    {
                        const char *text = SDL_GetClipboardText();
                        window->navigation->options().navigationType
                                = vts::NavigationType::FlyOver;
                        window->navigation->setPositionUrl(text);
                    }
                    catch(...)
                    {
                        // do nothing
                    }
                }
            }

            // subjective position
            {
                int subj = window->navigation->getSubjective();
                int prev = subj;
                nk_label(&ctx, "Type:", NK_TEXT_LEFT);
                nk_checkbox_label(&ctx, "subjective", &subj);
                if (subj != prev)
                    window->navigation->setSubjective(!!subj, true);
            }

            // srs
            {
                static const char *names[] = {
                    "Physical",
                    "Navigation",
                    "Public",
                };
                nk_label(&ctx, "Srs:", NK_TEXT_LEFT);
                if (nk_combo_begin_label(&ctx,
                                 names[positionSrs],
                                 nk_vec2(nk_widget_width(&ctx), 200)))
                {
                    nk_layout_row_dynamic(&ctx, 16, 1);
                    for (int i = 0; i < 3; i++)
                        if (nk_combo_item_label(&ctx, names[i], NK_TEXT_LEFT))
                            positionSrs = i;
                    nk_combo_end(&ctx);
                }
            }

            // position
            {
                nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
                double n[3];
                window->navigation->getPoint(n);
                try
                {
                    window->map->convert(n, n, Srs::Navigation,
                                         (Srs)positionSrs);
                }
                catch (const std::exception &)
                {
                    for (int i = 0; i < 3; i++)
                        n[i] = nan1();
                }
                nk_label(&ctx, "X:", NK_TEXT_LEFT);
                sprintf(buffer, "%.8f", n[0]);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Y:", NK_TEXT_LEFT);
                sprintf(buffer, "%.8f", n[1]);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Z:", NK_TEXT_LEFT);
                sprintf(buffer, "%.8f", n[2]);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "", NK_TEXT_LEFT);
                if (nk_button_label(&ctx, "Reset altitude"))
                {
                    window->navigation->options().navigationType
                            = vts::NavigationType::Quick;
                    window->navigation->resetAltitude();
                }
            }

            // rotation
            {
                float ratio2[] = { width * 0.4f, width * 0.3f, width * 0.3f };
                nk_layout_row(&ctx, NK_STATIC, 16, 3, ratio2);
                double n[3], c[3];
                window->navigation->getRotation(n);
                window->navigation->getRotationLimited(c);
                // yaw
                nk_label(&ctx, "Yaw:", NK_TEXT_LEFT);
                sprintf(buffer, "%5.1f", n[0]);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                sprintf(buffer, "%5.1f", c[0]);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                // pitch/tilt
                nk_label(&ctx, "Pitch:", NK_TEXT_LEFT);
                sprintf(buffer, "%5.1f", n[1]);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                sprintf(buffer, "%5.1f", c[1]);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                // roll
                nk_label(&ctx, "Roll:", NK_TEXT_LEFT);
                sprintf(buffer, "%5.1f", n[2]);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                sprintf(buffer, "%5.1f", c[2]);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                // reset
                nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
                nk_label(&ctx, "", NK_TEXT_LEFT);
                if (nk_button_label(&ctx, "Reset rotation"))
                {
                    window->navigation->setRotation({0,270,0});
                    window->navigation->options().navigationType
                            = vts::NavigationType::Quick;
                    window->navigation->resetNavigationMode();
                }
            }

            // view extent
            {
                nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
                nk_label(&ctx, "View extent:", NK_TEXT_LEFT);
                sprintf(buffer, "%10.1f",
                        window->navigation->getViewExtent());
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            }

            // fov
            {
                float ratio[] = { width * 0.4f,
                        width * 0.45f, width * 0.15f };
                nk_layout_row(&ctx, NK_STATIC, 16, 3, ratio);
                nk_label(&ctx, "Fov:", NK_TEXT_LEFT);
                window->navigation->setFov(nk_slide_float(&ctx, 10,
                                    window->navigation->getFov(), 100, 1));
                sprintf(buffer, "%5.1f", window->navigation->getFov());
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            }

            // output
            {
                nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
                nk_label(&ctx, "Output:", NK_TEXT_LEFT);
                if (nk_button_label(&ctx, "Copy to clipboard"))
                    SDL_SetClipboardText(
                            window->navigation->getPositionUrl().c_str());
            }

            // camera
            if (nk_tree_push(&ctx, NK_TREE_TAB, "Camera", NK_MINIMIZED))
            {
                float ratio[] = { width * 0.5f, width * 0.5f };
                nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
                const auto &c = window->camera->draws().camera;
                nk_label(&ctx, "Target Distance:", NK_TEXT_LEFT);
                sprintf(buffer, "%.8f", c.tagretDistance);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "View Extent:", NK_TEXT_LEFT);
                sprintf(buffer, "%.8f", c.viewExtent);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Altitude Over Ellipsoid:", NK_TEXT_LEFT);
                sprintf(buffer, "%.8f", c.altitudeOverEllipsoid);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Altitude Over Surface:", NK_TEXT_LEFT);
                sprintf(buffer, "%.8f", c.altitudeOverSurface);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_tree_pop(&ctx);
            }

            // auto movement
            if (nk_tree_push(&ctx, NK_TREE_TAB, "Auto", NK_MINIMIZED))
            {
                float ratio[] = { width * 0.4f, width * 0.6f };
                nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);

                for (int i = 0; i < 3; i++)
                {
                    nk_label(&ctx, i == 0 ? "Move:" : "", NK_TEXT_LEFT);
                    posAutoMotion[i] = nk_slide_float(&ctx, -3,
                                                posAutoMotion[i], 3, 0.1);
                }
                nk_label(&ctx, "Rotate:", NK_TEXT_LEFT);
                posAutoRotation = nk_slide_float(&ctx, -3,
                                posAutoRotation, 3, 0.1);
                window->navigation->pan(posAutoMotion.data());
                window->navigation->rotate({ posAutoRotation, 0, 0 });
                window->navigation->options().navigationType
                        = vts::NavigationType::Quick;
                nk_tree_pop(&ctx);
            }
        }
        nk_end(&ctx);
    }

    std::string labelWithCounts(const std::string &label,
                                std::size_t a, std::size_t b)
    {
        std::ostringstream ss;
        if (b == 0)
            ss << label << " (0)";
        else
            ss << label << " (" << a << " / " << b << ")";
        return ss.str();
    }

    bool prepareViewsBoundLayers(MapView::BoundLayerInfo::List &bl,
                                    uint32 &bid)
    {
        const std::vector<std::string> boundLayers
                = window->map->getResourceBoundLayers();
        if (nk_tree_push_id(&ctx, NK_TREE_NODE,
                labelWithCounts("Bound Layers",
                bl.size(), boundLayers.size()).c_str(),
                            NK_MINIMIZED, bid++))
        {
            struct Ender
            {
                nk_context *ctx;
                Ender(nk_context *ctx) : ctx(ctx) {};
                ~Ender() { nk_tree_pop(ctx); }
            } ender(&ctx);

            std::set<std::string> bls(boundLayers.begin(), boundLayers.end());
            float width = nk_window_get_content_region_size(&ctx).x - 70;

            // enabled layers
            bool changed = false;
            if (!bl.empty())
            {
                float ratio[] = { width * 0.7f, width * 0.3f, 20};
                nk_layout_row(&ctx, NK_STATIC, 16, 3, ratio);
                int idx = 0;
                for (auto &bn : bl)
                {
                    if (!nk_check_label(&ctx, bn.id.c_str(), 1))
                    {
                        bl.erase(bl.begin() + idx);
                        return true;
                    }
                    bls.erase(bn.id);

                    // alpha
                    double a2 = nk_slide_float(&ctx, 0.1, bn.alpha , 1, 0.1);
                    if (bn.alpha != a2)
                    {
                        bn.alpha = a2;
                        changed = true;
                    }

                    // arrows
                    if (idx > 0)
                    {
                        if (nk_button_label(&ctx, "^"))
                        {
                            std::swap(bl[idx - 1], bl[idx]);
                            return true;
                        }
                    }
                    else
                        nk_label(&ctx, "", NK_TEXT_LEFT);

                    idx++;
                }
            }

            // available layers
            if (!bls.empty())
            {
                nk_layout_row(&ctx, NK_STATIC, 16, 1, &width);
                for (auto &bn : bls)
                {
                    if (nk_check_label(&ctx, bn.c_str(), 0))
                    {
                        bl.push_back(MapView::BoundLayerInfo(bn));
                        return true;
                    }
                }
            }
            return changed;
        }
        return false;
    }

    uint32 currentMapConfig()
    {
        std::string current = window->map->getMapconfigPath();
        uint32 idx = 0;
        for (auto it : window->appOptions.paths)
        {
            if (it.mapConfig == current)
                return idx;
            idx++;
        }
        return 0;
    }

    void selectMapconfig(uint32 index)
    {
        window->marks.clear();
        if (index == (uint32)-1)
            index = window->appOptions.paths.size() - 1;
        else
            index = index % window->appOptions.paths.size();
        window->setMapConfigPath(window->appOptions.paths[index]);
    }

    void prepareViews()
    {
        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Views", nk_rect(530, 10, 350, 600), flags))
        {
            float width = nk_window_get_content_region_size(&ctx).x - 30;

            // mapconfig selector
            if (window->appOptions.paths.size() > 1)
            {
                // combo selector
                nk_layout_row(&ctx, NK_STATIC, 20, 1, &width);
                if (nk_combo_begin_label(&ctx,
                                 window->map->getMapconfigPath().c_str(),
                                 nk_vec2(nk_widget_width(&ctx), 200)))
                {
                    nk_layout_row_dynamic(&ctx, 16, 1);
                    for (int i = 0, e = window->appOptions.paths.size();
                         i < e; i++)
                    {
                        if (nk_combo_item_label(&ctx,
                                window->appOptions.paths[i].mapConfig.c_str(),
                                                NK_TEXT_LEFT))
                        {
                            selectMapconfig(i);
                            nk_combo_end(&ctx);
                            nk_end(&ctx);
                            return;
                        }
                    }
                    nk_combo_end(&ctx);
                }

                // buttons
                {
                    float ratio[] = { width * 0.5f, width * 0.5f };
                    nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
                    if (nk_button_label(&ctx, "< Prev"))
                    {
                        selectMapconfig(currentMapConfig() - 1);
                        nk_end(&ctx);
                        return;
                    }
                    if (nk_button_label(&ctx, "Next >"))
                    {
                        selectMapconfig(currentMapConfig() + 1);
                        nk_end(&ctx);
                        return;
                    }
                }
            }

            // loading?
            if (!window->map->getMapconfigAvailable())
            {
                nk_layout_row(&ctx, NK_STATIC, 16, 1, &width);
                nk_label(&ctx, "Loading...", NK_TEXT_LEFT);
                nk_end(&ctx);
                return;
            }

            // named view selector
            std::vector<std::string> names = window->map->getViewNames();
            if (names.size() > 1)
            {
                nk_layout_row(&ctx, NK_STATIC, 20, 1, &width);
                if (nk_combo_begin_label(&ctx,
                             window->map->getViewCurrent().c_str(),
                             nk_vec2(nk_widget_width(&ctx), 200)))
                {
                    nk_layout_row_dynamic(&ctx, 16, 1);
                    for (int i = 0, e = names.size(); i < e; i++)
                        if (nk_combo_item_label(&ctx, names[i].c_str(),
                                                NK_TEXT_LEFT))
                            window->map->setViewCurrent(names[i]);
                    nk_combo_end(&ctx);
                }
            }

            // current view
            bool viewChanged = false;
            MapView view = window->map->getViewData(
                        window->map->getViewCurrent());

            // surfaces
            const std::vector<std::string> surfaces
                    = window->map->getResourceSurfaces();
            if (nk_tree_push(&ctx, NK_TREE_TAB, labelWithCounts(
                    "Surfaces", view.surfaces.size(),
                    surfaces.size()).c_str(),
                             NK_MINIMIZED))
            {
                uint32 bid = 0;
                for (const std::string &sn : surfaces)
                {
                    nk_layout_row(&ctx, NK_STATIC, 16, 1, &width);
                    bool v1 = view.surfaces.find(sn) != view.surfaces.end();
                    bool v2 = nk_check_label(&ctx, sn.c_str(), v1);
                    if (v2)
                    {
                        // bound layers
                        MapView::SurfaceInfo &s = view.surfaces[sn];
                        viewChanged = viewChanged
                            || prepareViewsBoundLayers(s.boundLayers, bid);
                    }
                    else
                        view.surfaces.erase(sn);
                    if (v1 != v2)
                        viewChanged = true;
                }
                nk_tree_pop(&ctx);
            }

            // free layers
            const std::vector<std::string> freeLayers
                    = window->map->getResourceFreeLayers();
            if (nk_tree_push(&ctx, NK_TREE_TAB, labelWithCounts(
                    "Free Layers", view.freeLayers.size(),
                    freeLayers.size()).c_str(),
                             NK_MINIMIZED))
            {
                uint32 bid = 2000000000;
                for (const std::string &ln : freeLayers)
                {
                    nk_layout_row(&ctx, NK_STATIC, 16, 1, &width);
                    bool v1 = view.freeLayers.find(ln)
                                != view.freeLayers.end();
                    bool v2 = nk_check_label(&ctx, ln.c_str(), v1);
                    if (v2)
                    {
                        MapView::FreeLayerInfo &s = view.freeLayers[ln];
                        bool editableStyle = false;
                        bool editableGeodata = false;
                        switch (window->map->getResourceFreeLayerType(ln))
                        {
                        case FreeLayerType::TiledMeshes:
                        {
                            // bound layers
                            viewChanged = viewChanged
                                || prepareViewsBoundLayers(
                                        s.boundLayers, bid);
                        } break;
                        case FreeLayerType::MonolithicGeodata:
                            editableGeodata = true;
                            editableStyle = true;
                            break;
                        case FreeLayerType::TiledGeodata:
                            editableStyle = true;
                            break;
                        default:
                            break;
                        }
                        if (editableGeodata || editableStyle)
                        {
                            {
                                float ratio[] = { 15, (width - 15) * 0.5f,
                                                (width - 15) * 0.5f };
                                nk_layout_row(&ctx, NK_STATIC, 16, 3, ratio);
                            }
                            nk_label(&ctx, "", NK_TEXT_LEFT);
                            if (editableStyle)
                            {
                                if (nk_button_label(&ctx, "Style"))
                                {
                                    std::string v = window->map
                                        ->getResourceFreeLayerStyle(ln);
                                    v = editor(ln + ".style.json", v);
                                    window->map
                                        ->setResourceFreeLayerStyle(ln, v);
                                }
                            }
                            else
                                nk_label(&ctx, "", NK_TEXT_LEFT);
                            if (editableGeodata)
                            {
                                if (nk_button_label(&ctx, "Geodata"))
                                {
                                    std::string v = window->map
                                        ->getResourceFreeLayerGeodata(ln);
                                    v = editor(ln + ".geo.json", v);
                                    window->map
                                        ->setResourceFreeLayerGeodata(ln, v);
                                }
                            }
                            else
                                nk_label(&ctx, "", NK_TEXT_LEFT);
                        }
                    }
                    else
                        view.freeLayers.erase(ln);
                    if (v1 != v2)
                        viewChanged = true;
                }

                // fabricate geodata layer
                nk_layout_row(&ctx, NK_STATIC, 16, 1, &width);
                if (nk_button_label(&ctx, "Add Geodata Layer"))
                {
                    const std::set<std::string> fls(freeLayers.begin(),
                                                    freeLayers.end());
                    for (uint32 i = 1; true; i++)
                    {
                        std::ostringstream ss;
                        ss << "Geodata " << i;
                        std::string n = ss.str();
                        if (fls.count(n) == 0)
                        {
                            window->map->fabricateResourceFreeLayerGeodata(n);
                            break;
                        }
                    }
                }

                nk_tree_pop(&ctx);
            }

            if (viewChanged)
            {
                window->map->setViewData("", view);
                window->map->setViewCurrent("");
            }
        }

        // end window
        nk_end(&ctx);
    }

    void prepareMarks()
    {
        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Marks", nk_rect(1150, 10, 250, 400), flags))
        {
            std::vector<Mark> &marks = window->marks;
            float width = nk_window_get_content_region_size(&ctx).x - 15;
            float ratio[] = { width * 0.6f, width * 0.4f };
            nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
            char buffer[256];
            Mark *prev = nullptr;
            int i = 0;
            double length = 0;
            for (Mark &m : marks)
            {
                sprintf(buffer, "%d", (i + 1));
                nk_checkbox_label(&ctx, buffer, &m.open);
                double l = prev
                        ? vts::length(vec3(prev->coord - m.coord))
                        : 0;
                length += l;
                sprintf(buffer, "%.3f", l);
                nk_color c;
                c.r = 255 * m.color(0);
                c.g = 255 * m.color(1);
                c.b = 255 * m.color(2);
                c.a = 255;
                nk_label_colored(&ctx, buffer, NK_TEXT_RIGHT, c);
                if (m.open)
                {
                    double n[3] = { m.coord(0), m.coord(1), m.coord(2) };
                    try
                    {
                        window->map->convert(n, n, Srs::Physical,
                                             (Srs)positionSrs);
                    }
                    catch(...)
                    {
                        for (int i = 0; i < 3; i++)
                            n[i] = std::numeric_limits<double>::quiet_NaN();
                    }
                    sprintf(buffer, "%.8f", n[0]);
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                    if (nk_button_label(&ctx, "Go"))
                    {
                        double n[3] = { m.coord(0), m.coord(1), m.coord(2) };
                        window->map->convert(n, n, Srs::Physical,
                                                 Srs::Navigation);
                        window->navigation->setPoint(n);
                        window->navigation->options().navigationType
                                = vts::NavigationType::FlyOver;
                    }
                    sprintf(buffer, "%.8f", n[1]);
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                    nk_label(&ctx, "", NK_TEXT_RIGHT);
                    sprintf(buffer, "%.8f", n[2]);
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                    if (nk_button_label(&ctx, "Remove"))
                    {
                        marks.erase(marks.begin() + i);
                        break;
                    }
                }
                prev = &m;
                i++;
            }
            nk_label(&ctx, "Total:", NK_TEXT_LEFT);
            sprintf(buffer, "%.3f", length);
            nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            nk_label(&ctx, "", NK_TEXT_LEFT);
            if (nk_button_label(&ctx, "Clear all"))
                marks.clear();
        }
        nk_end(&ctx);
    }

    void prepareSearch()
    {
        if (search && window->map->statistics().renderTicks % 120 == 60)
        {
            try
            {
                double point[3];
                window->navigation->getPoint(point);
                search->updateDistances(point);
            }
            catch (...)
            {
                search.reset();
            }
        }

        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Search", nk_rect(1410, 10, 350, 500), flags))
        {
            float width = nk_window_get_content_region_size(&ctx).x - 30;
            if (!window->map->searchable())
            {
                nk_layout_row(&ctx, NK_STATIC, 20, 1, &width);
                nk_label(&ctx, "Search not available.", NK_TEXT_LEFT);
                nk_end(&ctx);
                return;
            }

            // search query
            {
                float ratio[] = { width * 0.15f, width * 0.85f };
                nk_layout_row(&ctx, NK_STATIC, 22, 2, ratio);
                nk_label(&ctx, "Query:", NK_TEXT_LEFT);
                int len = strlen(searchText);
                nk_edit_string(&ctx, NK_EDIT_FIELD | NK_EDIT_AUTO_SELECT,
                               searchText, &len,
                        MaxSearchTextLength - 1, nullptr);
                searchText[len] = 0;
                if (strcmp(searchText, searchTextPrev) != 0)
                {
                    if (nk_utf_len(searchText, len) >= 3)
                        search = window->map->search(searchText);
                    else
                        search.reset();
                    strcpy(searchTextPrev, searchText);
                }
            }

            // search results
            if (!search)
            {
                nk_end(&ctx);
                return;
            }

            if (!search->done)
            {
                nk_layout_row(&ctx, NK_STATIC, 20, 1, &width);
                nk_label(&ctx, "Searching...", NK_TEXT_LEFT);
                nk_end(&ctx);
                return;
            }

            if (search->results.empty())
            {
                nk_layout_row(&ctx, NK_STATIC, 20, 1, &width);
                nk_label(&ctx, "No results.", NK_TEXT_LEFT);
                nk_end(&ctx);
                return;
            }

            char buffer[200];
            std::vector<SearchItem> &res = search->results;
            int index = 0;
            for (auto &r : res)
            {
                float ratio[] = { width * 0.7f,
                            width * 0.18f, width * 0.12f };
                nk_layout_row(&ctx, NK_STATIC, 18, 3, ratio);

                // title
                nk_label(&ctx, r.title.c_str(), NK_TEXT_LEFT);

                // distance
                if (r.distance >= 1e3)
                    sprintf(buffer, "%.1lf km", r.distance / 1e3);
                else
                    sprintf(buffer, "%.1lf m", r.distance);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);

                // go button
                if (r.position[0] == r.position[0])
                {
                    if (nk_button_label(&ctx, "Go"))
                    {
                        double pr = window->map->celestialBody().majorRadius;
                        window->navigation->setSubjective(false, false);
                        window->navigation->setViewExtent(std::max(
                        6667.0 * pr / window->map->celestialBody().majorRadius,
                            r.radius * 2));
                        window->navigation->setRotation({0,270,0});
                        window->navigation->resetAltitude();
                        window->navigation->resetNavigationMode();
                        window->navigation->setPoint(r.position);
                        window->navigation->options().navigationType
                                = vts::NavigationType::FlyOver;
                    }
                }
                else
                    nk_label(&ctx, "", NK_TEXT_LEFT);

                // region
                if (nk_tree_push_id(&ctx, NK_TREE_NODE, r.region.c_str(),
                        NK_MINIMIZED, index))
                {
                    float ratio[] = { width * 0.2f, width * 0.8f };
                    nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
                    nk_label(&ctx, "Name:", NK_TEXT_LEFT);
                    nk_label(&ctx, r.displayName.c_str(), NK_TEXT_LEFT);
                    nk_label(&ctx, "Type:", NK_TEXT_LEFT);
                    nk_label(&ctx, r.type.c_str(), NK_TEXT_LEFT);
                    nk_label(&ctx, "Road:", NK_TEXT_LEFT);
                    nk_label(&ctx, r.road.c_str(), NK_TEXT_LEFT);
                    nk_label(&ctx, "City:", NK_TEXT_LEFT);
                    nk_label(&ctx, r.city.c_str(), NK_TEXT_LEFT);
                    nk_label(&ctx, "County:", NK_TEXT_LEFT);
                    nk_label(&ctx, r.county.c_str(), NK_TEXT_LEFT);
                    nk_label(&ctx, "State:", NK_TEXT_LEFT);
                    nk_label(&ctx, r.state.c_str(), NK_TEXT_LEFT);
                    nk_label(&ctx, "Number:", NK_TEXT_LEFT);
                    nk_label(&ctx, r.houseNumber.c_str(), NK_TEXT_LEFT);
                    nk_label(&ctx, "District:", NK_TEXT_LEFT);
                    nk_label(&ctx, r.stateDistrict.c_str(), NK_TEXT_LEFT);
                    nk_label(&ctx, "Country:", NK_TEXT_LEFT);
                    nk_label(&ctx, r.country.c_str(), NK_TEXT_LEFT);
                    nk_label(&ctx, "Code:", NK_TEXT_LEFT);
                    nk_label(&ctx, r.countryCode.c_str(), NK_TEXT_LEFT);
                    nk_label(&ctx, "Importance:", NK_TEXT_LEFT);
                    sprintf(buffer, "%lf", r.importance);
                    nk_label(&ctx, buffer, NK_TEXT_LEFT);
                    nk_label(&ctx, "Radius:", NK_TEXT_LEFT);
                    sprintf(buffer, "%lf", r.radius);
                    nk_label(&ctx, buffer, NK_TEXT_LEFT);
                    nk_tree_pop(&ctx);
                }
                index++;
            }
        }
        nk_end(&ctx);
    }

    void prepare(int, int)
    {
        prepareOptions();
        prepareStatistics();
        preparePosition();
        prepareViews();
        prepareMarks();
        prepareSearch();
        prepareFirst = false;
    }

    void render(int width, int height)
    {
        if (hideTheGui)
            return;
        prepare(width, height);
        dispatch(width, height);
    }

    static const int MaxSearchTextLength = 200;
    char searchText[MaxSearchTextLength];
    char searchTextPrev[MaxSearchTextLength];
    char positionInputText[MaxSearchTextLength];

    std::shared_ptr<Texture> fontTexture;
    std::shared_ptr<Texture> skinTexture;
    std::shared_ptr<Shader> shader;
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<SearchTask> search;

    GuiSkinMedia skinMedia;
    nk_context ctx;
    nk_font_atlas atlas;
    nk_font *font;
    nk_buffer cmds;
    nk_convert_config config;
    nk_draw_null_texture null;

    vec3 posAutoMotion;
    double posAutoRotation;
    double viewExtentLimitScaleMin;
    double viewExtentLimitScaleMax;

    int positionSrs;

    MainWindow *window;
    bool prepareFirst;
    bool hideTheGui;

    static const int MaxVertexMemory = 4 * 1024 * 1024;
    static const int MaxElementMemory = 4 * 1024 * 1024;
};

void MainWindow::Gui::initialize(MainWindow *window)
{
    impl = std::make_shared<GuiImpl>(window);
}

void MainWindow::Gui::render(int width, int height)
{
    impl->render(width, height);
}

void MainWindow::Gui::inputBegin()
{
    nk_input_begin(&impl->ctx);
}

bool MainWindow::Gui::input(SDL_Event &event)
{
    impl->handleEvent(&event);
    return nk_item_is_any_active(&impl->ctx);
}

void MainWindow::Gui::inputEnd()
{
    nk_input_end(&impl->ctx);
}

void MainWindow::Gui::finalize()
{
    impl.reset();
}

void MainWindow::Gui::visible(bool visible)
{
    impl->hideTheGui = !visible;
}
