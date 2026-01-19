#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QImage>

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

    // 重写关闭事件，用于程序退出时保存状态
    void closeEvent(QCloseEvent *event) override;

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

    void saveSession();   // 保存会话
    void loadSession();   // 加载会话信息并自动打开

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

private:
    // 渲染监视器，用于监听异步任务完成
    QFutureWatcher<QImage> m_renderWatcher;

    // 渲染槽函数
    void handleRenderFinished();

    // 记录渲染时的参数，防止异步竞争导致页面错乱
    int m_renderingPage = -1;
};

#endif // MAINWINDOW_H
