/*
 * MIT License
 *
 * Copyright (C) 2021-2023 by wangwenx190 (Yuhang Zhao)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "quickimageitem.h"
#include "quickimageitem_p.h"
#include <QtCore/qloggingcategory.h>
#include <QtGui/qpainter.h>
#include <QtGui/qimage.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qicon.h>

FRAMELESSHELPER_BEGIN_NAMESPACE

[[maybe_unused]] static Q_LOGGING_CATEGORY(lcQuickImageItem, "wangwenx190.framelesshelper.quick.quickimageitem")

#ifdef FRAMELESSHELPER_QUICK_NO_DEBUG_OUTPUT
#  define INFO QT_NO_QDEBUG_MACRO()
#  define DEBUG QT_NO_QDEBUG_MACRO()
#  define WARNING QT_NO_QDEBUG_MACRO()
#  define CRITICAL QT_NO_QDEBUG_MACRO()
#else
#  define INFO qCInfo(lcQuickImageItem)
#  define DEBUG qCDebug(lcQuickImageItem)
#  define WARNING qCWarning(lcQuickImageItem)
#  define CRITICAL qCCritical(lcQuickImageItem)
#endif

using namespace Global;

FRAMELESSHELPER_STRING_CONSTANT2(QrcPrefix, "qrc:")
FRAMELESSHELPER_STRING_CONSTANT2(FileSystemPrefix, ":")
FRAMELESSHELPER_STRING_CONSTANT2(UrlPrefix, ":///")
FRAMELESSHELPER_STRING_CONSTANT2(FilePathPrefix, ":/")

QuickImageItemPrivate::QuickImageItemPrivate(QuickImageItem *q) : QObject(q)
{
    Q_ASSERT(q);
    if (!q) {
        return;
    }
    q_ptr = q;
}

QuickImageItemPrivate::~QuickImageItemPrivate() = default;

QuickImageItemPrivate *QuickImageItemPrivate::get(QuickImageItem *q)
{
    Q_ASSERT(q);
    if (!q) {
        return nullptr;
    }
    return q->d_func();
}

const QuickImageItemPrivate *QuickImageItemPrivate::get(const QuickImageItem *q)
{
    Q_ASSERT(q);
    if (!q) {
        return nullptr;
    }
    return q->d_func();
}

void QuickImageItemPrivate::fromUrl(const QUrl &value, QPainter *painter) const
{
    Q_ASSERT(value.isValid());
    Q_ASSERT(painter);
    if (!value.isValid() || !painter) {
        return;
    }
    fromString((value.isLocalFile() ? value.toLocalFile() : value.toString()), painter);
}

void QuickImageItemPrivate::fromString(const QString &value, QPainter *painter) const
{
    Q_ASSERT(!value.isEmpty());
    Q_ASSERT(painter);
    if (value.isEmpty() || !painter) {
        return;
    }
    return fromPixmap(QPixmap([&value]() -> QString {
        // For most Qt classes, the "qrc:///" prefix won't be recognized as a valid
        // file system path, unless it accepts a QUrl object. For QString constructors
        // we can only use ":/" to represent the file system path.
        QString path = value;
        if (path.startsWith(kQrcPrefix, Qt::CaseInsensitive)) {
            path.replace(kQrcPrefix, kFileSystemPrefix, Qt::CaseInsensitive);
        }
        if (path.startsWith(kUrlPrefix, Qt::CaseInsensitive)) {
            path.replace(kUrlPrefix, kFilePathPrefix, Qt::CaseInsensitive);
        }
        return path;
    }()), painter);
}

void QuickImageItemPrivate::fromImage(const QImage &value, QPainter *painter) const
{
    Q_ASSERT(!value.isNull());
    Q_ASSERT(painter);
    if (value.isNull() || !painter) {
        return;
    }
    fromPixmap(QPixmap::fromImage(value), painter);
}

void QuickImageItemPrivate::fromPixmap(const QPixmap &value, QPainter *painter) const
{
    Q_ASSERT(!value.isNull());
    Q_ASSERT(painter);
    if (value.isNull() || !painter) {
        return;
    }
    const QRectF paintRect = paintArea();
    const QSize paintSize = paintRect.size().toSize();
    painter->drawPixmap(paintRect.topLeft(), (value.size() == paintSize ? value : value.scaled(paintSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
}

void QuickImageItemPrivate::fromIcon(const QIcon &value, QPainter *painter) const
{
    Q_ASSERT(!value.isNull());
    Q_ASSERT(painter);
    if (value.isNull() || !painter) {
        return;
    }
    fromPixmap(value.pixmap(paintArea().size().toSize()), painter);
}

QRectF QuickImageItemPrivate::paintArea() const
{
    Q_Q(const QuickImageItem);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    const QSizeF size = q->size();
#else
    const QSizeF size = {q->width(), q->height()};
#endif
    return {QPointF(0, 0), size};
}

QuickImageItem::QuickImageItem(QQuickItem *parent)
    : QQuickPaintedItem(parent), d_ptr(new QuickImageItemPrivate(this))
{
    setAntialiasing(true);
    setSmooth(true);
    setMipmap(true);
    setClip(true);
}

QuickImageItem::~QuickImageItem() = default;

void QuickImageItem::paint(QPainter *painter)
{
    Q_ASSERT(painter);
    if (!painter) {
        return;
    }
    Q_D(QuickImageItem);
    if (!d->source.isValid() || d->source.isNull()) {
        return;
    }
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing |
        QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    switch (d->source.userType()) {
    case QMetaType::QUrl:
        d->fromUrl(d->source.toUrl(), painter);
        break;
    case QMetaType::QString:
        d->fromString(d->source.toString(), painter);
        break;
    case QMetaType::QImage:
        d->fromImage(qvariant_cast<QImage>(d->source), painter);
        break;
    case QMetaType::QPixmap:
        d->fromPixmap(qvariant_cast<QPixmap>(d->source), painter);
        break;
    case QMetaType::QIcon:
        d->fromIcon(qvariant_cast<QIcon>(d->source), painter);
        break;
    default:
        WARNING << "Unsupported type:" << d->source.typeName();
        break;
    }
    painter->restore();
}

QVariant QuickImageItem::source() const
{
    Q_D(const QuickImageItem);
    return d->source;
}

void QuickImageItem::setSource(const QVariant &value)
{
    Q_ASSERT(value.isValid());
    Q_ASSERT(!value.isNull());
    if (!value.isValid() || value.isNull()) {
        return;
    }
    Q_D(QuickImageItem);
    if (d->source == value) {
        return;
    }
    d->source = value;
    update();
    Q_EMIT sourceChanged();
}

void QuickImageItem::classBegin()
{
    QQuickPaintedItem::classBegin();
}

void QuickImageItem::componentComplete()
{
    QQuickPaintedItem::componentComplete();
}

FRAMELESSHELPER_END_NAMESPACE
