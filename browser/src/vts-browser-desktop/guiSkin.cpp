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

#include <vts-browser/resources.hpp>
#include <vts-renderer/classes.hpp>

#include "guiSkin.hpp"

using vts::renderer::Texture;

GuiSkinMedia::GuiSkinMedia()
{
    memset(this, 0, sizeof(*this));
}

void initializeGuiSkin(nk_context &ctx, GuiSkinMedia &media,
                       std::shared_ptr<Texture> &tex)
{
    { // load texture
        tex = std::make_shared<Texture>();
        vts::GpuTextureSpec spec(
        vts::readInternalMemoryBuffer("data/textures/gwen.png"));
        spec.filterMode = vts::GpuTextureSpec::FilterMode::Linear;
        tex->load(spec);
        media.skin = tex->getId();
    }

    media.check = nk_subimage_id(media.skin, 512,512, nk_rect(464,32,15,15));
    media.check_cursor = nk_subimage_id(media.skin, 512,512, nk_rect(450,34,11,11));
    media.option = nk_subimage_id(media.skin, 512,512, nk_rect(464,64,15,15));
    media.option_cursor = nk_subimage_id(media.skin, 512,512, nk_rect(451,67,9,9));
    media.header = nk_subimage_id(media.skin, 512,512, nk_rect(128,0,127,24));
    media.window = nk_subimage_id(media.skin, 512,512, nk_rect(128,23,127,104));
    media.scrollbar_inc_button = nk_subimage_id(media.skin, 512,512, nk_rect(464,256,15,15));
    media.scrollbar_inc_button_hover = nk_subimage_id(media.skin, 512,512, nk_rect(464,320,15,15));
    media.scrollbar_dec_button = nk_subimage_id(media.skin, 512,512, nk_rect(464,224,15,15));
    media.scrollbar_dec_button_hover = nk_subimage_id(media.skin, 512,512, nk_rect(464,288,15,15));
    media.button = nk_subimage_id(media.skin, 512,512, nk_rect(384,336,127,31));
    media.button_hover = nk_subimage_id(media.skin, 512,512, nk_rect(384,368,127,31));
    media.button_active = nk_subimage_id(media.skin, 512,512, nk_rect(384,400,127,31));
    media.tab_minimize = nk_subimage_id(media.skin, 512,512, nk_rect(451, 99, 9, 9));
    media.tab_maximize = nk_subimage_id(media.skin, 512,512, nk_rect(467,99,9,9));
    media.slider = nk_subimage_id(media.skin, 512,512, nk_rect(418,33,11,14));
    media.slider_hover = nk_subimage_id(media.skin, 512,512, nk_rect(418,49,11,14));
    media.slider_active = nk_subimage_id(media.skin, 512,512, nk_rect(418,64,11,14));

    ctx.style.text.color = nk_rgb(200,200,200);

    { // window
        ctx.style.window.header.label_normal
                = ctx.style.window.header.label_hover
                = ctx.style.window.header.label_active
                = ctx.style.text.color;
    }

    { // button
        ctx.style.button.text_normal = ctx.style.button.text_hover
                = ctx.style.button.text_active = ctx.style.text.color;
    }

    { // combo
        ctx.style.combo.label_normal = ctx.style.combo.label_hover
                = ctx.style.combo.label_active = ctx.style.text.color;
        ctx.style.contextual_button.text_normal
                = ctx.style.contextual_button.text_hover
                = ctx.style.contextual_button.text_active
                = ctx.style.text.color;
    }

    { // checkbox toggle
        nk_style_toggle &toggle = ctx.style.checkbox;
        toggle.normal = nk_style_item_image(media.check);
        toggle.hover = nk_style_item_image(media.check);
        toggle.active = nk_style_item_image(media.check);
        toggle.cursor_normal = nk_style_item_image(media.check_cursor);
        toggle.cursor_hover = nk_style_item_image(media.check_cursor);
        toggle.text_normal = toggle.text_hover = toggle.text_active
                = ctx.style.text.color;
    }

    { // option toggle
        nk_style_toggle &toggle = ctx.style.option;
        toggle.normal = nk_style_item_image(media.option);
        toggle.hover = nk_style_item_image(media.option);
        toggle.active = nk_style_item_image(media.option);
        toggle.cursor_normal = nk_style_item_image(media.option_cursor);
        toggle.cursor_hover = nk_style_item_image(media.option_cursor);
        toggle.text_normal = toggle.text_hover = toggle.text_active
                = ctx.style.text.color;
    }

    { // slider
        ctx.style.slider.cursor_normal = nk_style_item_image(media.slider);
        ctx.style.slider.cursor_hover = nk_style_item_image(media.slider);
        ctx.style.slider.cursor_active = nk_style_item_image(media.slider);
        ctx.style.slider.cursor_size = nk_vec2(9,15);
        ctx.style.slider.bar_height = 3;
    }
}
