#include <QLineEdit>
#include <QVBoxLayout>

#include "SConfig.h"
#include "SSettings.h"
#include "SPluginEditor.h"
#include "SButton.h"
#include "utils.h"

SPluginEditor* SPluginEditor::m_instance = nullptr;

SPluginEditor* SPluginEditor::editor(QWidget *parent)
{
    SDEBUG
    if (!m_instance)
    {
        m_instance = new SPluginEditor(parent);
    }
    return m_instance;
}

SPluginEditor::SPluginEditor(QWidget *parent)
{
    SDEBUG
    this->setParent(parent);
    initGui();
}

void SPluginEditor::initGui()
{
    SDEBUG
    if (!m_iconLabel)   m_iconLabel = new QLabel(tr("Icon"), this);
    if (!m_nameLabel)   m_nameLabel = new QLabel(tr("Name"), this);
    if (!m_tipLabel)    m_tipLabel = new QLabel(tr("Tip"), this);
    if (!m_scriptLabel) m_scriptLabel = new QLabel(tr("Script"), this);
    if (!m_eButton)     m_eButton = new SButton(tr("Save"), this);
    if (!m_cButton)     m_cButton = new SButton(tr("Create"), this);

    if (!m_iconContainor)   m_iconContainor = new QLabel(this);
    if (!m_nameEdit)        m_nameEdit = new QLineEdit(this);
    if (!m_tipEdit)         m_tipEdit = new QLineEdit(this);
    if (!m_scriptEdit)      m_scriptEdit = new QLineEdit(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_iconContainor);
    layout->addWidget(m_nameLabel);
    layout->addWidget(m_nameEdit);
    layout->addWidget(m_tipLabel);
    layout->addWidget(m_tipEdit);
    layout->addWidget(m_scriptLabel);
    layout->addWidget(m_scriptEdit);
    layout->addWidget(m_eButton);
    layout->addWidget(m_cButton);

    this->setLayout(layout);

    QObject::connect(m_eButton, &SButton::clicked, this, [this]() {
        SPluginInfo &info = *this->m_editingItem->pluginInfo();
        info.name = this->m_nameEdit->text();
        info.script = this->m_scriptEdit->text();
        info.tip = this->m_tipEdit->text();
        info.icon = this->m_icon;

        emit this->edited(this->m_editingItem);
        this->close();
    });
    QObject::connect(m_cButton, &SButton::clicked, this, [this]() {
        SPluginInfo *info = new SPluginInfo(this->m_nameEdit->text(), 
            this->m_scriptEdit->text(), m_icon, 0, this->m_tipEdit->text(), true);
        emit this->created(info);
        this->close();
    });
}

void SPluginEditor::edit(SPluginItem *item)
{
    SDEBUG
    m_editingItem = item;

    SPluginInfo &info = *item->pluginInfo();
    m_iconContainor->setPixmap(info.icon);
    m_nameEdit->setText(info.name);
    m_tipEdit->setText(info.tip);
    m_scriptEdit->setText(info.script);
    m_cButton->setVisible(false);
    m_eButton->setVisible(true);

    this->setWindowTitle(tr("Edit") + ' ' + info.name);
    this->show();
}

void SPluginEditor::create()
{
    SDEBUG
    // m_iconContainor->setPixmap(defaultPixmap);
    m_nameEdit->setText("");
    m_tipEdit->setText("");
    m_scriptEdit->setText("");
    m_eButton->setVisible(false);
    m_cButton->setVisible(true);

    this->setWindowTitle(tr("Create New Plugin"));
    this->show();
}