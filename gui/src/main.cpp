#include <QApplication>
#include <QMainWindow>
#include <QLabel>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QMainWindow window;
    window.setWindowTitle("IntuiCAM");

    QLabel* label = new QLabel("Welcome to IntuiCAM");
    label->setAlignment(Qt::AlignCenter);
    window.setCentralWidget(label);

    window.resize(800, 600);
    window.show();
    return app.exec();
}
