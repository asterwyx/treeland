#include "treelandcapture.h"

#include <qwdisplay.h>
#include <woutputrenderwindow.h>
#include <wbufferrenderer_p.h>

QuickCaptureSource *QuickCaptureContext::source() const
{
    return m_captureSource;
}

void QuickCaptureContext::setSource(QuickCaptureSource *source)
{
    if (m_captureSource == source)
        return;
    m_captureSource = source;
    m_handle->sendSourceReady(source->captureRegion(), source->sourceType());
    Q_EMIT sourceChanged();
    Q_EMIT finishSelect();
}

WSurface *QuickCaptureContext::mask() const
{
    return m_handle->mask;
}

bool QuickCaptureContext::freeze() const
{
    return m_handle->freeze;
}

bool QuickCaptureContext::withCursor() const
{
    return m_handle->withCursor;
}

QuickCaptureSource::CaptureSourceHint QuickCaptureContext::sourceHint() const
{
    return QuickCaptureSource::CaptureSourceHint(m_handle->sourceHint);
}

QuickCaptureContext::QuickCaptureContext(treeland_capture_context_v1 *h, QObject *parent)
    : QObject(parent)
    , m_handle(h)
{
    connect(h, &treeland_capture_context_v1::selectSource, this, &QuickCaptureContext::onSelectSource);
}

void QuickCaptureContext::onSelectSource()
{
    treeland_capture_context_v1 *context = qobject_cast<treeland_capture_context_v1*>(sender());
    Q_ASSERT(context); // Sender must be context.
    Q_EMIT selectInfoReady();
}

void QuickCaptureContext::sendSourceFailed(SourceFailure failure)
{
    m_handle->sendSourceFailed(failure);
    Q_EMIT finishSelect();
}

QuickCaptureManager::QuickCaptureManager(QObject *parent)
    : WQuickWaylandServerInterface(parent)
    , m_manager(nullptr)
    , m_captureContextModel(new CaptureContextModel(this))
    , m_contextInSelection(nullptr)
{
}

WOutputRenderWindow *QuickCaptureManager::outputRenderWindow() const
{
    return m_outputRenderWindow;
}

void QuickCaptureManager::setOutputRenderWindow(WOutputRenderWindow *renderWindow)
{
    if (m_outputRenderWindow == renderWindow) {
        return;
    }
    m_outputRenderWindow = renderWindow;
    Q_EMIT outputRenderWindowChanged();
}

WServerInterface *QuickCaptureManager::create()
{
    m_manager = new treeland_capture_manager_v1(server()->handle()->handle(), this);
    connect(m_manager,
            &treeland_capture_manager_v1::newCaptureContext,
            this,
            [this](treeland_capture_context_v1 *context) {
                auto quickContext = new QuickCaptureContext(context, this);
                m_captureContextModel->addContext(quickContext);
                connect(context,
                        &treeland_capture_context_v1::beforeDestroy,
                        quickContext,
                        [this, quickContext] {
                            m_captureContextModel->removeContext(quickContext);
                            quickContext->deleteLater();
                        });
                connect(quickContext,
                        &QuickCaptureContext::selectInfoReady,
                        this,
                        &QuickCaptureManager::onCaptureContextSelectSource);
            });
    return new WServerInterface(m_manager, m_manager->global);
}

void QuickCaptureManager::onCaptureContextSelectSource()
{
    QuickCaptureContext *context = qobject_cast<QuickCaptureContext*>(sender());
    Q_ASSERT(context); // Sender must be context.
    if (contextInSelection()) {
        context->sendSourceFailed(QuickCaptureContext::SelectorBusy);
        return;
    }
    connect(context, &QuickCaptureContext::destroyed, this, [this, context]{
        clearContextInSelection(context);
    });
    connect(context, &QuickCaptureContext::finishSelect, this, [this, context] {
        clearContextInSelection(context);
    });
    m_contextInSelection = context;
    Q_EMIT contextInSelectionChanged();
    if (context->freeze()) {
        Q_ASSERT(m_outputRenderWindow);

        // TODO: freeze all client, do not send feedback to them

        // Get last frame buffer
        auto renderer = m_outputRenderWindow->currentRenderer();
        auto buffer = renderer->currentBuffer();

        // For freeze selection, we must capture output contents here.
    }
}

void QuickCaptureManager::clearContextInSelection(QuickCaptureContext *context)
{
    if (m_contextInSelection == context) {
        m_contextInSelection = nullptr;
        Q_EMIT contextInSelectionChanged();
    }
}

// QuickCaptureSession::QuickCaptureSession(
//     treeland_capture_session_v1 *h, QObject *parent)
//     : QuickCaptureContext(parent)
//     , m_handle(h)
//     , m_captureSource(nullptr)
// {
//     handleMap.append({h, this});
// }

// QuickCaptureSession *QuickCaptureSession::fromHandle(treeland_capture_session_v1 *handle)
// {
//     for (const auto &entry : handleMap) {
//         if (entry.first == handle)
//             return entry.second;
//     }
//     return nullptr;
// }

// QuickCaptureOnceContext::QuickCaptureOnceContext(
//     treeland_capture_context_v1 *h, QObject *parent)
//     : QuickCaptureContext(parent)
//     , m_handle(h)
//     , m_captureSource(nullptr)
// {
//     handleMap.append({h, this});
//     connect(m_handle, &treeland_capture_context_v1::copy, this, &QuickCaptureOnceContext::copy);
// }


// void QuickCaptureOnceContext::setSource(QuickCaptureSource *source)
// {
//     if (m_captureSource == source)
//         return;
//     m_captureSource = source;
//     Q_EMIT sourceChanged();
//     QRect captureRegion = m_captureSource->captureRegion();
//     m_handle->sendCaptureInfo(captureRegion.x(), captureRegion.y(), captureRegion.width(), captureRegion.height(), m_captureSource->sourceType());

// }

// QuickCaptureOnceContext *QuickCaptureOnceContext::fromHandle(treeland_capture_context_v1 *handle)
// {
//     for (const auto &entry : handleMap) {
//         if (entry.first == handle)
//             return entry.second;
//     }
//     return nullptr;
// }

// void QuickCaptureOnceContext::copy(QW_NAMESPACE::QWBuffer *buffer)
// {

// }


// void QuickCaptureSession::setSource(QuickCaptureSource *source)
// {

// }

// // QuickOutputCaptureSource::QuickOutputCaptureSource(WOutput *output, QObject *parent)
// //     : QuickCaptureSource(parent)
// // { }


CaptureContextModel::CaptureContextModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CaptureContextModel::rowCount(const QModelIndex &parent) const
{
    return m_captureContexts.size();
}

QVariant CaptureContextModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_captureContexts.size())
        return QVariant();
    switch (role) {
    case ContextRole:
        return QVariant::fromValue(m_captureContexts.at(index.row()));
    }
    return QVariant();
}


QHash<int, QByteArray> CaptureContextModel::roleNames() const
{
    return QHash<int, QByteArray>{
        {ContextRole, QByteArrayLiteral("context")}
    };
}

void CaptureContextModel::addContext(QuickCaptureContext *context)
{
    beginInsertRows(QModelIndex(), m_captureContexts.size(), m_captureContexts.size() + 1);
    m_captureContexts.push_back(context);
    endInsertRows();
}

void CaptureContextModel::removeContext(QuickCaptureContext *context)
{
    auto index = m_captureContexts.indexOf(context);
    beginRemoveRows(QModelIndex(), index, index);
    m_captureContexts.remove(index);
    endRemoveRows();
}



QuickCaptureSourceRegion::QuickCaptureSourceRegion(WOutputRenderWindow *renderWindow, QRect region, QObject *parent)
    : QuickCaptureSource(parent)
{

}


QW_NAMESPACE::QWBuffer *QuickCaptureSourceRegion::sourceDMABuffer()
{
    // For region, directly return dma buffer of render window
    auto renderWindowBuffer = m_renderWindow->currentRenderer()->currentRenderer()
}

void QuickCaptureSourceRegion::copyBuffer(QW_NAMESPACE::QWBuffer *)
{

}


QRect QuickCaptureSourceRegion::captureRegion()
{
    return m_region;
}

QuickCaptureSource::CaptureSourceType QuickCaptureSourceRegion::sourceType()
{
    return QuickCaptureSource::Region;
}
