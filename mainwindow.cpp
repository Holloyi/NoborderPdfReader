#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "pdfdocument.h"

#include <QFileDialog>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QShortcut>
#include <QPixmap>
#include <QMenuBar>
#include <QStatusBar>

#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QIntValidator>

#include <QApplication>
#include <QEvent>
#include <QCursor>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ✅ 真·沉浸式：无标题栏/无边框
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    if (menuBar()) menuBar()->hide();
    if (statusBar()) statusBar()->hide();

    if (ui->centralwidget) {
        ui->centralwidget->setContentsMargins(0,0,0,0);
        if (ui->centralwidget->layout())
            ui->centralwidget->layout()->setContentsMargins(0,0,0,0);
    }

    // 显示区
    ui->lblReader->setAlignment(Qt::AlignCenter);
    ui->lblReader->setScaledContents(false);
    ui->lblReader->setFocusPolicy(Qt::NoFocus);
    ui->lblReader->setText(QStringLiteral(
        "按 Ctrl+O 打开 PDF\n"
        "←/→ 或滚轮翻页，Ctrl+滚轮缩放\n"
        "Tab 显示/隐藏页码条，Ctrl+G 跳页，Esc 退出\n"
        "（无边框窗口：顶部可拖动，边缘可缩放）"
    ));

    // 页码条（默认隐藏）
    setupPageBar();
    updatePageBar();
    setPageBarVisible(false);

    // ✅ 全局拦截 Ctrl+滚轮：必须装在 qApp 上
    qApp->installEventFilter(this);

    // Ctrl+O 打开
    auto *scOpen = new QShortcut(QKeySequence::Open, this);
    connect(scOpen, &QShortcut::activated, this, &MainWindow::openPdf);

    // Esc 退出
    auto *scEsc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(scEsc, &QShortcut::activated, this, [this](){ close(); });

    // Tab：切换页码条显示隐藏
    auto *scToggleBar = new QShortcut(QKeySequence(Qt::Key_Tab), this);
    connect(scToggleBar, &QShortcut::activated, this, &MainWindow::togglePageBar);

    // Ctrl+G：显示页码条并聚焦输入框
    auto *scGoto = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_G), this);
    connect(scGoto, &QShortcut::activated, this, [this](){
        setPageBarVisible(true);
        if (m_pageEdit) {
            m_pageEdit->setFocus();
            m_pageEdit->selectAll();
        }
    });
}

MainWindow::~MainWindow()
{
    // 解除全局过滤器（严谨）
    qApp->removeEventFilter(this);
    delete ui;
}

void MainWindow::openPdf()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择 PDF 文件"),
        QString(),
        QStringLiteral("PDF Files (*.pdf)")
    );
    if (file.isEmpty()) return;

    if (!m_pdf) m_pdf = new PdfDocument(this);

    if (!m_pdf->load(file)) {
        ui->lblReader->setText(QStringLiteral("PDF 加载失败（可能是路径/权限/格式问题）"));
        return;
    }

    m_currentFile = file;
    m_currentPage = 0;
    m_scale = 1.5;

    renderCurrentPage();
}

void MainWindow::renderCurrentPage()
{
    if (!m_pdf) return;

    const int total = m_pdf->pageCount();
    if (total <= 0) {
        ui->lblReader->setText(QStringLiteral("PDF 页数为 0"));
        updatePageBar();
        return;
    }

    m_currentPage = qBound(0, m_currentPage, total - 1);

    QImage img = m_pdf->renderPage(m_currentPage, m_scale);
    if (img.isNull()) {
        ui->lblReader->setText(QStringLiteral("渲染失败"));
        updatePageBar();
        return;
    }

    QPixmap pm = QPixmap::fromImage(img);
    ui->lblReader->setPixmap(pm.scaled(
        ui->lblReader->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    ));

    setWindowTitle(QString("Page %1 / %2").arg(m_currentPage + 1).arg(total));
    updatePageBar();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (m_pdf && !m_currentFile.isEmpty()) {
        renderCurrentPage();
    } else {
        updatePageBar();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (!m_pdf || m_pdf->pageCount() <= 0) {
        QMainWindow::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_Right || event->key() == Qt::Key_PageDown || event->key() == Qt::Key_Down) {
        m_currentPage = qMin(m_currentPage + 1, m_pdf->pageCount() - 1);
        renderCurrentPage();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_PageUp || event->key() == Qt::Key_Up) {
        m_currentPage = qMax(m_currentPage - 1, 0);
        renderCurrentPage();
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (!m_pdf || m_pdf->pageCount() <= 0) {
        QMainWindow::wheelEvent(event);
        return;
    }

    // 普通滚轮：翻页（Ctrl+滚轮缩放由 eventFilter 处理）
    int delta = event->angleDelta().y();
    if (delta == 0) delta = event->pixelDelta().y();

    if (delta < 0) {
        m_currentPage = qMin(m_currentPage + 1, m_pdf->pageCount() - 1);
        renderCurrentPage();
        event->accept();
        return;
    } else if (delta > 0) {
        m_currentPage = qMax(m_currentPage - 1, 0);
        renderCurrentPage();
        event->accept();
        return;
    }

    QMainWindow::wheelEvent(event);
}

// ✅ 全局拦截 Ctrl+滚轮：保证不触发“焦点变化”
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

    if (event->type() != QEvent::Wheel) {
        return QMainWindow::eventFilter(watched, event);
    }

    // 必须已打开 PDF
    if (!m_pdf || m_pdf->pageCount() <= 0) {
        return QMainWindow::eventFilter(watched, event);
    }

    auto *we = static_cast<QWheelEvent*>(event);

    // 只拦截 Ctrl + 滚轮
    if (!(we->modifiers() & Qt::ControlModifier)) {
        return QMainWindow::eventFilter(watched, event);
    }

    // 只在鼠标位于本窗口时生效
    const QPoint globalPos = QCursor::pos();
    const QPoint localPos = mapFromGlobal(globalPos);
    if (!rect().contains(localPos)) {
        return QMainWindow::eventFilter(watched, event);
    }

    int delta = we->angleDelta().y();
    if (delta == 0) delta = we->pixelDelta().y();

    // 即使 delta=0，也吃掉事件，避免系统/Qt 用 Ctrl+滚轮做焦点导航
    if (delta == 0) {
        we->accept();
        return true;
    }

    if (delta > 0) m_scale *= 1.10;
    else m_scale /= 1.10;

    m_scale = qBound(0.3, m_scale, 6.0);
    renderCurrentPage();

    we->accept();
    return true; // ✅ 吃掉事件
}

// ---------------- 页码条 ----------------

void MainWindow::setupPageBar()
{
    m_pageBar = new QWidget(this);
    m_pageBar->setObjectName("pageBar");
    m_pageBar->setFocusPolicy(Qt::NoFocus);

    m_pageBar->setStyleSheet(
        "#pageBar { background: rgba(0,0,0,150); border-radius: 10px; }"
        "QLabel { color: white; }"
        "QLineEdit { color: white; background: rgba(255,255,255,30); border: 1px solid rgba(255,255,255,60); "
        "border-radius: 6px; padding: 4px 6px; }"
    );

    auto *layout = new QHBoxLayout(m_pageBar);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(8);

    m_pageLabel = new QLabel(m_pageBar);
    m_pageLabel->setText(QStringLiteral("Page - / -"));
    m_pageLabel->setFocusPolicy(Qt::NoFocus);

    m_pageEdit = new QLineEdit(m_pageBar);
    m_pageEdit->setPlaceholderText(QStringLiteral("跳页"));
    m_pageEdit->setFixedWidth(70);
    m_pageEdit->setAlignment(Qt::AlignCenter);
    m_pageEdit->setClearButtonEnabled(true);

    m_pageValidator = new QIntValidator(1, 1, m_pageEdit);
    m_pageEdit->setValidator(m_pageValidator);

    layout->addWidget(m_pageLabel);
    layout->addWidget(m_pageEdit);

    m_pageBar->adjustSize();
    m_pageBar->hide();
    m_pageBarVisible = false;

    connect(m_pageEdit, &QLineEdit::returnPressed, this, [this](){
        bool ok = false;
        int p = m_pageEdit->text().toInt(&ok);
        if (ok) jumpToPage(p);
    });
}

void MainWindow::updatePageBar()
{
    const int total = (m_pdf ? m_pdf->pageCount() : 0);
    const int current1 = (total > 0 ? (m_currentPage + 1) : 0);

    if (m_pageLabel) {
        m_pageLabel->setText(QStringLiteral("Page %1 / %2").arg(current1).arg(total));
    }

    if (m_pageEdit) {
        m_pageEdit->setEnabled(total > 0);
    }
    if (m_pageValidator) {
        m_pageValidator->setRange(1, qMax(1, total));
    }

    if (m_pageBar) {
        const int margin = 16;
        const int safe = 10;

        m_pageBar->adjustSize();
        int x = width() - m_pageBar->width() - margin - safe;
        int y = height() - m_pageBar->height() - margin - safe;
        m_pageBar->move(qMax(0, x), qMax(0, y));

        if (m_pageBarVisible) m_pageBar->raise();
    }
}

void MainWindow::jumpToPage(int page1Based)
{
    if (!m_pdf) return;
    int total = m_pdf->pageCount();
    if (total <= 0) return;

    int target = qBound(1, page1Based, total) - 1;
    if (target == m_currentPage) return;

    m_currentPage = target;
    renderCurrentPage();
}

void MainWindow::setPageBarVisible(bool visible)
{
    m_pageBarVisible = visible;
    if (!m_pageBar) return;

    if (visible) {
        updatePageBar();
        m_pageBar->show();
        m_pageBar->raise();
    } else {
        m_pageBar->hide();
    }
}

void MainWindow::togglePageBar()
{
    setPageBarVisible(!m_pageBarVisible);
}

// ---------------- 无边框缩放/拖动（Windows） ----------------
#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    MSG *msg = static_cast<MSG*>(message);

    if (msg->message == WM_NCHITTEST) {
        const LONG border = 8;
        RECT winRect;
        GetWindowRect(HWND(winId()), &winRect);

        const long x = GET_X_LPARAM(msg->lParam);
        const long y = GET_Y_LPARAM(msg->lParam);

        const bool resizeLeft   = x >= winRect.left  && x <  winRect.left  + border;
        const bool resizeRight  = x <  winRect.right && x >= winRect.right - border;
        const bool resizeTop    = y >= winRect.top   && y <  winRect.top   + border;
        const bool resizeBottom = y <  winRect.bottom&& y >= winRect.bottom - border;

        if (resizeTop && resizeLeft)     { *result = HTTOPLEFT; return true; }
        if (resizeTop && resizeRight)    { *result = HTTOPRIGHT; return true; }
        if (resizeBottom && resizeLeft)  { *result = HTBOTTOMLEFT; return true; }
        if (resizeBottom && resizeRight) { *result = HTBOTTOMRIGHT; return true; }

        if (resizeLeft)   { *result = HTLEFT; return true; }
        if (resizeRight)  { *result = HTRIGHT; return true; }
        if (resizeTop)    { *result = HTTOP; return true; }
        if (resizeBottom) { *result = HTBOTTOM; return true; }

        // 顶部 40px 可拖动
        POINT pt = { (LONG)x, (LONG)y };
        ScreenToClient(HWND(winId()), &pt);
        if (pt.y >= 0 && pt.y < 40) {
            *result = HTCAPTION;
            return true;
        }

        *result = HTCLIENT;
        return true;
    }

    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif
