#include <renderer/map.h>
#include <renderer/statistics.h>
#include <renderer/options.h>
#include "mainWindow.h"
#include <nuklear.h>
#include <GLFW/glfw3.h>

class GuiImpl
{
public:
    GuiImpl(MainWindow *window) : window(window)
    {
        
    }
    
    ~GuiImpl()
    {
        
    }
    
    MainWindow *window;
};

void MainWindow::Gui::initialize(MainWindow *window)
{
    impl = std::make_shared<GuiImpl>(window);
}

void MainWindow::Gui::render()
{
    
}

void MainWindow::Gui::finalize()
{
    impl.reset();
}
