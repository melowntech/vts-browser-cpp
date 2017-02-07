#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), timer(parent)
{
    ui->setupUi(this);

    timer.connect(&timer, &QTimer::timeout, this, &MainWindow::onTimer);
    timer.start(30);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onTimer()
{
    this->update();
}
