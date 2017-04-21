#ifndef GUISKIN_H_qwdfvgfwdhsbde
#define GUISKIN_H_qwdfvgfwdhsbde

#include <memory>

#include <glad/glad.h>
#include <nuklear.h>

struct GuiSkinMedia
{
    GLint skin;
    struct nk_image menu;
    struct nk_image check;
    struct nk_image check_cursor;
    struct nk_image option;
    struct nk_image option_cursor;
    struct nk_image header;
    struct nk_image window;
    struct nk_image scrollbar_inc_button;
    struct nk_image scrollbar_inc_button_hover;
    struct nk_image scrollbar_dec_button;
    struct nk_image scrollbar_dec_button_hover;
    struct nk_image button;
    struct nk_image button_hover;
    struct nk_image button_active;
    struct nk_image tab_minimize;
    struct nk_image tab_maximize;
    struct nk_image slider;
    struct nk_image slider_hover;
    struct nk_image slider_active;
    
    GuiSkinMedia();
};

void initializeGuiSkin(nk_context &ctx, GuiSkinMedia &media,
                       std::shared_ptr<class GpuTextureImpl> &tex);

#endif
