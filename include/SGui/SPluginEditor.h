/**
 * @file SPluginEditor.h
 * @author Ccslykx (ccslykx@outlook.com)
 * @brief 负责在设置创建/编辑插件的信息。
 * @version 0.1
 * @date 2023-06-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPointer>
#include <QWidget>

#include "SButton.h"
#include "SPluginInfo.h"

class QCloseEvent;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;
class QFrame;
class QPlainTextEdit;
class QProgressBar;
class QScrollArea;

class SPluginEditor : public QWidget
{
    Q_OBJECT

public:
    static SPluginEditor* editor(QWidget *parent = (QWidget*)nullptr);
    void edit(SPluginInfo*);
    void create();

signals:
    void created(SPluginInfo*);
    void editingOpened();
    void editingFinished();

private:
    enum class StatusKind
    {
        Hidden,
        Loading,
        Success,
        Error
    };

    explicit SPluginEditor(QWidget *parent = (QWidget*)nullptr);

    void initGui();
    void initialize();
    void submit();
    void requestBack();
    void finishEditing();
    bool confirmDiscardChanges();
    void updateValidation();
    void setFieldError(QWidget *field, QLabel *label, const QString &message);
    void setDirty(bool dirty);
    void updateTipCounter();
    void showStatus(const QString &message, StatusKind kind);
    void hideStatus();
    bool loadIcon(const QString &path);
    void resetIcon();
    void updateIconPreview();
    void testCommand();
    void refreshTheme(bool force = false,
                      Qt::ColorScheme scheme = Qt::ColorScheme::Unknown);
    void setDragActive(bool active);

    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    static SPluginEditor *m_instance;

    QLabel          *m_titleLabel = nullptr;
    QLabel          *m_subtitleLabel = nullptr;
    QLabel          *m_dirtyLabel = nullptr;
    QLabel          *m_iconLabel = nullptr;
    QLabel          *m_iconHintLabel = nullptr;
    QLabel          *m_nameLabel = nullptr;
    QLabel          *m_nameErrorLabel = nullptr;
    QLabel          *m_tipLabel = nullptr;
    QLabel          *m_tipCounterLabel = nullptr;
    QLabel          *m_scriptLabel = nullptr;
    QLabel          *m_scriptHelpLabel = nullptr;
    QLabel          *m_scriptErrorLabel = nullptr;
    QLabel          *m_statusLabel = nullptr;

    QFrame          *m_formCard = nullptr;
    QFrame          *m_statusWidget = nullptr;
    QProgressBar    *m_statusProgress = nullptr;
    QScrollArea     *m_formScrollArea = nullptr;

    SButton         *m_submitButton = nullptr;
    SButton         *m_testButton = nullptr;
    SButton         *m_cancelButton = nullptr;
    SButton         *m_iconContainor = nullptr;
    SButton         *m_resetIconButton = nullptr;
    SButton         *m_insertVariableButton = nullptr;
    SButton         *m_insertUrlEncodedButton = nullptr;

    QLineEdit       *m_nameEdit = nullptr;
    QLineEdit       *m_tipEdit = nullptr;
    QPlainTextEdit  *m_scriptEdit = nullptr;

    QPixmap          m_icon;
    QPointer<SPluginInfo> m_editingInfo;
    quint64          m_sessionId = 0;
    bool             m_editMode = false;
    bool             m_iconChanged = false;
    bool             m_dirty = false;
    bool             m_updatingFields = false;
    bool             m_nameTouched = false;
    bool             m_scriptTouched = false;
    bool             m_testStarting = false;
    bool             m_dragActive = false;
    bool             m_darkStyle = false;
    bool             m_styleInitialized = false;
    Qt::ColorScheme  m_pendingColorScheme = Qt::ColorScheme::Unknown;
};
