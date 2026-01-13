#ifndef PDFDOCUMENT_H
#define PDFDOCUMENT_H

#include <QObject>
#include <QImage>
#include <QSizeF>
#include <QString>
#include <QFile>

#include "fpdfview.h"

class PdfDocument : public QObject
{
    Q_OBJECT
public:
    explicit PdfDocument(QObject *parent = nullptr);
    ~PdfDocument();

    bool load(const QString &filePath);
    int pageCount() const;
    QSizeF pageSize(int pageIndex) const;
    QImage renderPage(int pageIndex, double renderScale);

private:
    void closeCurrent();

private:
    FPDF_DOCUMENT m_doc = nullptr;

    // ✅ 用 QFile 读取，彻底绕开中文路径问题
    QFile *m_file = nullptr;

    // ✅ 给 FPDF_LoadCustomDocument 用的 file access
    FPDF_FILEACCESS m_access{};
};

#endif // PDFDOCUMENT_H
