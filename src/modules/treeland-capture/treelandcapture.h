// Copyright (C) 2023 rewine <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "impl/treelandcapturev1impl.h"

#include <wglobal.h>
#include <wquickwaylandserver.h>
#include <woutput.h>
#include <wxdgsurface.h>
#include <wxdgsurfaceitem.h>

#include <QRect>
#include <QAbstractListModel>

WAYLIB_SERVER_BEGIN_NAMESPACE
class WOutputRenderWindow;
WAYLIB_SERVER_END_NAMESPACE

WAYLIB_SERVER_USE_NAMESPACE
class QuickCaptureSource : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(CaptureSource)
public:

    enum CaptureSourceType {
        Output = TREELAND_CAPTURE_CONTEXT_V1_SOURCE_TYPE_OUTPUT,
        Window = TREELAND_CAPTURE_CONTEXT_V1_SOURCE_TYPE_WINDOW,
        Region = TREELAND_CAPTURE_CONTEXT_V1_SOURCE_TYPE_REGION,
        Surface = 0x8
    };
    Q_FLAG(CaptureSourceType)
    Q_DECLARE_FLAGS(CaptureSourceHint, CaptureSourceType)


    QuickCaptureSource(QObject *parent = nullptr) : QObject(parent) { }

    /**
     * @brief DMA buffer of source, there are three cases
     * 1. output - output's dma buffer
     * 2. window - window's dma buffer
     * 3. region - output's dma buffer
     *
     * @return QW_NAMESPACE::QWBuffer*
     */
    virtual QW_NAMESPACE::QWBuffer *sourceDMABuffer() = 0;

    /**
     * @brief copyBuffer render captured contents to a buffer
     * @param buffer buffer prepared by client
     */
    virtual void copyBuffer(QW_NAMESPACE::QWBuffer *buffer) = 0;

    // Capture area relative to the whole viewport
    virtual QRect captureRegion() = 0;

    virtual CaptureSourceType sourceType() = 0;

};
class QuickCaptureContext;
class CaptureContextModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    CaptureContextModel(QObject *parent = nullptr);
    enum CaptureContextRole {
        ContextRole = Qt::UserRole + 1
    };
    Q_ENUM(CaptureContextRole)
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void addContext(QuickCaptureContext *context);
    void removeContext(QuickCaptureContext *context);
private:
    QList<QuickCaptureContext *> m_captureContexts;
};
class QuickCaptureFrame;
class QuickCaptureSession;
class QuickCaptureContext : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(CaptureContext)
    QML_UNCREATABLE("Only created in c++")
    Q_PROPERTY(WSurface *mask READ mask NOTIFY selectInfoReady FINAL)
    Q_PROPERTY(bool freeze READ freeze NOTIFY selectInfoReady FINAL)
    Q_PROPERTY(bool withCursor READ withCursor NOTIFY selectInfoReady FINAL)
    Q_PROPERTY(QuickCaptureSource::CaptureSourceHint sourceHint READ sourceHint NOTIFY selectInfoReady FINAL)
    Q_PROPERTY(QuickCaptureSource *source READ source WRITE setSource NOTIFY sourceChanged FINAL)
public:
    QuickCaptureSource *source() const;
    void setSource(QuickCaptureSource *source);

    WSurface *mask() const;
    bool freeze() const;
    bool withCursor() const;
    QuickCaptureSource::CaptureSourceHint sourceHint() const;

public:

    enum SourceFailure {
        SelectorBusy = TREELAND_CAPTURE_CONTEXT_V1_SOURCE_FAILURE_SELECTOR_BUSY,
        Other = TREELAND_CAPTURE_CONTEXT_V1_SOURCE_FAILURE_OTHER
    };
    Q_ENUM(SourceFailure)

    QuickCaptureContext(treeland_capture_context_v1 *h, QObject *parent = nullptr);
    Q_INVOKABLE void sendSourceFailed(SourceFailure failure);

    Q_INVOKABLE bool hintType(QuickCaptureSource::CaptureSourceType type) { return sourceHint().testFlag(type); }

Q_SIGNALS:
    void sourceChanged();
    void finishSelect();
    void selectInfoReady();

private:
    void onSelectSource();

    treeland_capture_context_v1 *const m_handle;
    QuickCaptureSource *m_captureSource;
    // QuickCaptureSession *m_session;
    // QuickCaptureFrame *m_frame;
};


// class QuickCaptureOnceContext : public QuickCaptureContext
// {
//     Q_OBJECT
// public:
//     QuickCaptureOnceContext(treeland_capture_context_v1 *h, QObject *parent = nullptr);
//     bool freezeOutput() const override { return true; }
//     bool withCursor() const override { return m_handle->withCursor; }
//     QuickCaptureSource::CaptureSourceHint sourceHint() const override { return QuickCaptureSource::CaptureSourceHint(m_handle->sourceHint); }
//     QuickCaptureSource *source() const override { return m_captureSource; }
//     void setSource(QuickCaptureSource *source) override;
//     static QuickCaptureOnceContext *fromHandle(treeland_capture_context_v1 *handle);


// private Q_SLOTS:
//     void copy(QW_NAMESPACE::QWBuffer* buffer);

// private:
//     treeland_capture_context_v1 *const m_handle;
//     QuickCaptureSource *m_captureSource;
//     static QList<QPair<treeland_capture_context_v1*, QuickCaptureOnceContext*>> handleMap;
// };

// class QuickCaptureSession : public QuickCaptureContext
// {
//     Q_OBJECT
// public:
//     QuickCaptureSession(treeland_capture_session_v1 *h, QObject *parent = nullptr);
//     static QuickCaptureSession *fromHandle(treeland_capture_session_v1 *handle);

//     QuickCaptureSource *source() const override { return m_captureSource; }
//     void setSource(QuickCaptureSource *source) override;

// private:
//     treeland_capture_session_v1 *const m_handle;
//     QuickCaptureSource *m_captureSource;
//     static QList<QPair<treeland_capture_session_v1*, QuickCaptureSession*>> handleMap;
// };

class QuickCaptureManager : public WQuickWaylandServerInterface
{
    Q_OBJECT
    QML_NAMED_ELEMENT(CaptureManager)
    Q_PROPERTY(CaptureContextModel *contextModel READ contextModel CONSTANT)
    Q_PROPERTY(QuickCaptureContext *contextInSelection READ contextInSelection NOTIFY contextInSelectionChanged FINAL)
    Q_PROPERTY(WOutputRenderWindow *outputRenderWindow READ outputRenderWindow WRITE setOutputRenderWindow NOTIFY outputRenderWindowChanged FINAL)

public:
    QuickCaptureManager(QObject *parent = nullptr);
    CaptureContextModel *contextModel() const { return m_captureContextModel; }
    QuickCaptureContext *contextInSelection() const { return m_contextInSelection; }
    WOutputRenderWindow *outputRenderWindow() const;
    void setOutputRenderWindow(WOutputRenderWindow *renderWindow);

Q_SIGNALS:
    void contextInSelectionChanged();
    void newCaptureContext(QuickCaptureContext *context);
    void outputRenderWindowChanged();

protected:
    WServerInterface *create() override;

private Q_SLOTS:
    void onCaptureContextSelectSource();
    void clearContextInSelection(QuickCaptureContext *context);

private:
    treeland_capture_manager_v1 *m_manager;
    CaptureContextModel *m_captureContextModel;
    QuickCaptureContext *m_contextInSelection;
    WOutputRenderWindow *m_outputRenderWindow;
    /**
     * @brief m_frozenOutputRenderWindowBuffer frozen output contents
     * note that output contents may still update after frozen client because
     * selector will also display something, so we need to save output render
     * window contents here for later use. For output, simply crop the buffer(maybe?)
     */
    QW_NAMESPACE::QWBuffer *m_frozenOutputRenderWindowBuffer;
};

// class QuickOutputCaptureSource : public QuickCaptureSource
// {
// public:
//     QuickOutputCaptureSource(WOutput *output, QObject *parent = nullptr);

//     QImage render() override;
//     QRect captureRegion() override;
//     CaptureSourceType sourceType() override { return Output; }
//     virtual QW_NAMESPACE::QWBuffer *sourceDMABuffer() override;

// private:
//     WOutput *output;
// };

class QuickCaptureSourceRegion : public QuickCaptureSource
{
    Q_OBJECT
public:
    QuickCaptureSourceRegion(WOutputRenderWindow *renderWindow, QRect region, QObject *parent = nullptr);


private:
    QPointer<WOutputRenderWindow> m_renderWindow;
    QRect m_region;

    // QuickCaptureSource interface
public:
    QW_NAMESPACE::QWBuffer *sourceDMABuffer() override;
    void copyBuffer(QW_NAMESPACE::QWBuffer *) override;
    QRect captureRegion() override;
    CaptureSourceType sourceType() override;
};
