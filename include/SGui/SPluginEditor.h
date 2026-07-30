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

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPointer>

#include "SButton.h"
#include "SPluginInfo.h"



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
    explicit SPluginEditor(QWidget *parent = (QWidget*)nullptr);
    void initGui();
    void initialize();
    void updateValidation();
    void showValidation(const QString &message, bool error);
    bool loadIcon(const QString &path);
    void testCommand();
    void closeEvent(QCloseEvent*) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;

private:
    static SPluginEditor *m_instance;

    QLabel      *m_titleLabel = nullptr;
    QLabel      *m_iconLabel = nullptr;
    QLabel      *m_nameLabel = nullptr;
    QLabel      *m_tipLabel = nullptr;
    QLabel      *m_scriptLabel = nullptr;
    QLabel      *m_validationLabel = nullptr;
    SButton     *m_eButton = nullptr; // edit button
    SButton     *m_cButton = nullptr; // create button
    SButton     *m_testButton = nullptr;
    SButton     *m_cancelButton = nullptr;

    SButton     *m_iconContainor = nullptr;
    QString     m_iconPath = "";
    QPixmap     m_icon;
    QLineEdit   *m_nameEdit = nullptr;
    QLineEdit   *m_tipEdit = nullptr;
    QLineEdit   *m_scriptEdit = nullptr;

    QPointer<SPluginInfo> m_editingInfo;
};
