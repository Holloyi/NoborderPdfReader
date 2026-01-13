#include "pdfdocument.h"

#include <QDebug>
#include <QtGlobal>

// ---- Custom file callbacks ----
static int MyGetBlock(void* param,
                      unsigned long position,
                      unsigned char* out_buffer,
                      unsigned long size)
{
    QFile* f = static_cast<QFile*>(param);
    if (!f || !f->isOpen()) return 0;

    if (!f->seek(position)) return 0;

    qint64 n = f->read(reinterpret_cast<char*>(out_buffer), size);
    return (n == static_cast<qint64>(size)) ? 1 : 0;
}


PdfDocument::PdfDocument(QObject *parent)
    : QObject(parent)
{
    FPDF_InitLibrary();
}

PdfDocument::~PdfDocument()
{
    closeCurrent();
    FPDF_DestroyLibrary();
}

void PdfDocument::closeCurrent()
{
    if (m_doc) {
        FPDF_CloseDocument(m_doc);
        m_doc = nullptr;
    }

    if (m_file) {
        if (m_file->isOpen()) m_file->close();
        delete m_file;
        m_file = nullptr;
    }

    m_access = FPDF_FILEACCESS{};
}

bool PdfDocument::load(const QString &filePath)
{
    closeCurrent();

    m_file = new QFile(filePath);
    if (!m_file->open(QIODevice::ReadOnly)) {
        qWarning() << "Open file failed:" << filePath << m_file->errorString();
        delete m_file;
        m_file = nullptr;
        return false;
    }

    m_access.m_FileLen = static_cast<unsigned long>(m_file->size());
    m_access.m_GetBlock = &MyGetBlock;
    m_access.m_Param = m_file;

    // ✅ 不传路径，直接从文件流加载：中文路径/中文名永远没问题
    m_doc = FPDF_LoadCustomDocument(&m_access, nullptr);
    if (!m_doc) {
        unsigned long err = FPDF_GetLastError();
        qWarning() << "FPDF_LoadCustomDocument failed, error=" << err << "path=" << filePath;
        closeCurrent();
        return false;
    }

    return true;
}

int PdfDocument::pageCount() const
{
    return m_doc ? FPDF_GetPageCount(m_doc) : 0;
}

QSizeF PdfDocument::pageSize(int pageIndex) const
{
    if (!m_doc) return QSizeF();

    FPDF_PAGE page = FPDF_LoadPage(m_doc, pageIndex);
    if (!page) return QSizeF();

    QSizeF s(FPDF_GetPageWidth(page), FPDF_GetPageHeight(page));
    FPDF_ClosePage(page);
    return s;
}

QImage PdfDocument::renderPage(int pageIndex, double renderScale)
{
    if (!m_doc) return QImage();
    renderScale = qBound(0.1, renderScale, 20.0);

    FPDF_PAGE page = FPDF_LoadPage(m_doc, pageIndex);
    if (!page) return QImage();

    const int w = qMax(1, int(FPDF_GetPageWidth(page) * renderScale));
    const int h = qMax(1, int(FPDF_GetPageHeight(page) * renderScale));

    QImage img(w, h, QImage::Format_ARGB32);
    img.fill(Qt::white);

    FPDF_BITMAP bm = FPDFBitmap_CreateEx(
        w, h,
        FPDFBitmap_BGRA,
        img.bits(),
        img.bytesPerLine()
    );

    FPDF_RenderPageBitmap(bm, page, 0, 0, w, h, 0, 0);

    FPDFBitmap_Destroy(bm);
    FPDF_ClosePage(page);

    return img;
}
