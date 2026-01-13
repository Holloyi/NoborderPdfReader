#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class PdfDocument;

class QWidget;
class QLabel;
class QLineEdit;
class QIntValidator;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // 键盘翻页
    void keyPressEvent(QKeyEvent *event) override;

    // 普通滚轮翻页（Ctrl+滚轮缩放在 eventFilter 里全局处理）
    void wheelEvent(QWheelEvent *event) override;

    // 窗口缩放后重新适配显示
    void resizeEvent(QResizeEvent *event) override;

    // ✅ 全局拦截 Ctrl+滚轮（避免焦点变化/控件抢事件）
    bool eventFilter(QObject *watched, QEvent *event) override;

#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

private:
    void openPdf();
    void renderCurrentPage();

    // 页码条
    void setupPageBar();
    void updatePageBar();
    void jumpToPage(int page1Based);
    void togglePageBar();
    void setPageBarVisible(bool visible);

private:
    Ui::MainWindow *ui;

    PdfDocument *m_pdf = nullptr;
    QString m_currentFile;
    int m_currentPage = 0;
    double m_scale = 1.5;

    // 页码条控件
    QWidget *m_pageBar = nullptr;
    QLabel *m_pageLabel = nullptr;
    QLineEdit *m_pageEdit = nullptr;
    QIntValidator *m_pageValidator = nullptr;
    bool m_pageBarVisible = false;
};

#endif // MAINWINDOW_H
