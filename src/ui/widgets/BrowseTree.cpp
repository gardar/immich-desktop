#include "ui/widgets/BrowseTree.h"

#include "ui/FolderPaths.h"

#include <QEasingCurve>
#include <QHeaderView>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTreeWidgetItemIterator>

namespace Aurora {
namespace {

constexpr int kKindRole = Qt::UserRole;
constexpr int kValueRole = Qt::UserRole + 1;

const QString kAlbum = QStringLiteral("album");
const QString kTag = QStringLiteral("tag");
const QString kFolder = QStringLiteral("folder");
const QString kFolderGroup = QStringLiteral("folderGroup");
const QString kNote = QStringLiteral("note");
const QString kRetry = QStringLiteral("retry");

} // namespace

BrowseTree::BrowseTree(BrowseSection section, QWidget *parent)
    : QTreeWidget(parent)
    , m_section(section)
    , m_heightAnimation(new QPropertyAnimation(this, "maximumHeight", this))
{
    setObjectName(QStringLiteral("browseTree"));
    setHeaderHidden(true);
    setColumnCount(1);
    setIndentation(16);
    // Flat album lists need no disclosure column; tags/folders keep nested guides.
    setRootIsDecorated(section == BrowseSection::Tags || section == BrowseSection::Folders);
    setUniformRowHeights(true);
    setFrameShape(QFrame::NoFrame);
    setExpandsOnDoubleClick(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTextElideMode(Qt::ElideRight);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    header()->setStretchLastSection(true);
    setMinimumHeight(0);
    setMaximumHeight(0);
    setFixedHeight(0);
    hide();
    addNote(tr("Loading…"));

    m_heightAnimation->setDuration(160);
    m_heightAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_heightAnimation, &QPropertyAnimation::finished, this, [this] {
        if (m_open) {
            setMaximumHeight(QWIDGETSIZE_MAX);
            setFixedHeight(contentHeight());
        } else {
            hide();
            setFixedHeight(0);
            setMaximumHeight(0);
        }
    });

    connect(this, &QTreeWidget::itemClicked, this,
            [this](QTreeWidgetItem *item) { activate(item); });
    connect(this, &QTreeWidget::itemExpanded, this, &BrowseTree::updateHeight);
    connect(this, &QTreeWidget::itemCollapsed, this, &BrowseTree::updateHeight);
}

BrowseSection BrowseTree::section() const
{
    return m_section;
}

void BrowseTree::setAlbums(const QList<ImmichAlbum> &albums)
{
    clearItems();
    m_albums.clear();
    if (albums.isEmpty())
        addNote(tr("No albums"));
    for (const ImmichAlbum &album : albums) {
        auto *item = new QTreeWidgetItem(this, {album.name});
        item->setData(0, kKindRole, kAlbum);
        item->setData(0, kValueRole, album.id);
        item->setToolTip(0, tr("%1 · %n item(s)", nullptr, album.assetCount).arg(album.name));
        m_albums.insert(album.id, album);
    }
    updateHeight();
    selectValue(m_selectedValue);
}

void BrowseTree::setTags(const QList<ImmichTag> &tags)
{
    clearItems();
    m_tags.clear();
    if (tags.isEmpty())
        addNote(tr("No tags"));

    QHash<QString, QTreeWidgetItem *> items;
    for (const ImmichTag &tag : tags) {
        auto *item = new QTreeWidgetItem({tag.name});
        item->setData(0, kKindRole, kTag);
        item->setData(0, kValueRole, tag.id);
        item->setToolTip(0, tag.path);
        items.insert(tag.id, item);
        m_tags.insert(tag.id, tag);
    }
    for (const ImmichTag &tag : tags) {
        if (QTreeWidgetItem *parent = items.value(tag.parentId))
            parent->addChild(items.value(tag.id));
        else
            addTopLevelItem(items.value(tag.id));
    }
    updateHeight();
    selectValue(m_selectedValue);
}

void BrowseTree::setFolderPaths(const QStringList &paths)
{
    clearItems();
    if (paths.isEmpty()) {
        addNote(tr("No folders"));
        updateHeight();
        return;
    }

    const QString root = folderTreeRoot(paths);
    QHash<QString, QTreeWidgetItem *> nodes;
    for (int i = 0; i < paths.size(); ++i) {
        QTreeWidgetItem *parent = nullptr;
        QString key;
        const QStringList segments =
            folderRelativePath(paths.at(i), root).split(u'/', Qt::SkipEmptyParts);
        for (const QString &segment : segments) {
            key += u'/' + segment;
            QTreeWidgetItem *node = nodes.value(key);
            if (!node) {
                node = parent ? new QTreeWidgetItem(parent, {segment})
                              : new QTreeWidgetItem(this, {segment});
                node->setData(0, kKindRole, kFolderGroup);
                node->setToolTip(0, key.mid(1));
                nodes.insert(key, node);
            }
            parent = node;
        }
        if (parent) {
            parent->setData(0, kKindRole, kFolder);
            parent->setData(0, kValueRole, paths.at(i));
        }
    }
    updateHeight();
    selectValue(m_selectedValue);
}

void BrowseTree::setUnavailable(bool permissionDenied)
{
    static const QHash<BrowseSection, QString> permissions = {
        {BrowseSection::Albums, QStringLiteral("album.read")},
        {BrowseSection::Tags, QStringLiteral("tag.read")},
        {BrowseSection::Folders, QStringLiteral("folder.read")}};
    clearItems();
    QTreeWidgetItem *note =
        addNote(permissionDenied
                    ? tr("Needs the %1 permission · click to retry")
                          .arg(permissions.value(m_section))
                    : tr("Couldn't be loaded · click to retry"));
    note->setData(0, kKindRole, kRetry);
    updateHeight();
}

void BrowseTree::showLoading()
{
    clearItems();
    addNote(tr("Loading…"));
    updateHeight();
}

void BrowseTree::selectValue(const QString &value)
{
    m_selectedValue = value;
    clearSelection();
    if (value.isEmpty())
        return;
    for (QTreeWidgetItemIterator it(this); *it; ++it) {
        if ((*it)->data(0, kValueRole).toString() == value) {
            for (QTreeWidgetItem *parent = (*it)->parent(); parent; parent = parent->parent())
                parent->setExpanded(true);
            (*it)->setSelected(true);
            updateHeight();
            emit itemSelected();
            return;
        }
    }
}

QRect BrowseTree::selectedRect() const
{
    const QList<QTreeWidgetItem *> selected = selectedItems();
    return selected.isEmpty() ? QRect() : visualItemRect(selected.first());
}

QSize BrowseTree::sizeHint() const
{
    int rows = 0;
    for (QTreeWidgetItemIterator it(const_cast<BrowseTree *>(this)); *it; ++it) {
        bool visible = true;
        for (QTreeWidgetItem *parent = (*it)->parent(); parent && visible;
             parent = parent->parent())
            visible = parent->isExpanded();
        if (visible)
            ++rows;
    }
    const int rowHeight = qMax(sizeHintForRow(0), fontMetrics().height() + 10);
    return QSize(QTreeWidget::sizeHint().width(), rows * rowHeight + 2 * frameWidth());
}

void BrowseTree::clearItems()
{
    clear();
}

QTreeWidgetItem *BrowseTree::addNote(const QString &text)
{
    auto *note = new QTreeWidgetItem(this, {text});
    note->setData(0, kKindRole, kNote);
    note->setFlags(Qt::ItemIsEnabled);
    note->setForeground(0, palette().color(QPalette::PlaceholderText));
    return note;
}

void BrowseTree::activate(QTreeWidgetItem *item)
{
    const QString kind = item->data(0, kKindRole).toString();
    const QString value = item->data(0, kValueRole).toString();
    if (kind == kFolderGroup) {
        item->setExpanded(!item->isExpanded());
    } else if (kind == kAlbum) {
        emit albumRequested(m_albums.value(value));
    } else if (kind == kTag) {
        emit tagRequested(m_tags.value(value));
    } else if (kind == kFolder) {
        emit folderRequested(value, item->toolTip(0));
    } else if (kind == kRetry) {
        showLoading();
        emit reloadRequested();
    }
}

int BrowseTree::contentHeight() const
{
    return sizeHint().height();
}

bool BrowseTree::isOpen() const
{
    return m_open;
}

void BrowseTree::setOpen(bool open, bool animate)
{
    if (m_open == open &&
        ((open && isVisible() && m_heightAnimation->state() != QAbstractAnimation::Running) ||
         (!open && !isVisible())))
        return;
    m_open = open;
    applyOpenState(animate);
}

void BrowseTree::applyOpenState(bool animate)
{
    m_heightAnimation->stop();
    const int target = m_open ? contentHeight() : 0;
    const int current = isVisible() ? height() : 0;

    if (!animate) {
        if (m_open) {
            setMaximumHeight(QWIDGETSIZE_MAX);
            setFixedHeight(target);
            show();
        } else {
            hide();
            setFixedHeight(0);
            setMaximumHeight(0);
        }
        return;
    }

    // Let maximumHeight clip the list; unlock any previous fixed height first.
    setMinimumHeight(0);
    setMaximumHeight(current);
    if (m_open)
        show();

    m_heightAnimation->setStartValue(current);
    m_heightAnimation->setEndValue(target);
    m_heightAnimation->start();
}

void BrowseTree::updateHeight()
{
    if (!m_open)
        return;
    if (m_heightAnimation->state() == QAbstractAnimation::Running) {
        m_heightAnimation->setEndValue(contentHeight());
        return;
    }
    setMaximumHeight(QWIDGETSIZE_MAX);
    setFixedHeight(contentHeight());
}

void BrowseTree::drawBranches(QPainter *painter, const QRect &rect,
                              const QModelIndex &index) const
{
    // Subtle L / | connectors for nested tags and folders (Albums stay a flat guide).
    if (!rootIsDecorated())
        return;

    painter->save();
    QPen pen(palette().color(QPalette::Mid));
    pen.setWidth(1);
    painter->setPen(pen);

    const int midY = rect.center().y();
    const int indent = indentation();
    QModelIndex walk = index;
    for (int level = 0; walk.isValid(); walk = walk.parent(), ++level) {
        const int x = rect.right() - indent * level - indent / 2;
        const bool lastSibling = !walk.siblingAtRow(walk.row() + 1).isValid();
        if (level == 0) {
            painter->drawLine(x, rect.top(), x, lastSibling ? midY : rect.bottom());
            painter->drawLine(x, midY, rect.right() - 2, midY);
        } else if (!lastSibling) {
            painter->drawLine(x, rect.top(), x, rect.bottom());
        }
    }
    painter->restore();
}

} // namespace Aurora
