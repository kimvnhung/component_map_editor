#include "LabelRenderPass.h"

#include "LabelTextureNode.h"

#include <QRectF>
#include <QSGNode>

namespace LabelRenderPass {

QVector<LabelTextureNode *> collectLabelNodes(QSGNode *labelsRootNode)
{
    QVector<LabelTextureNode *> labelNodes;
    if (!labelsRootNode)
        return labelNodes;

    for (QSGNode *child = labelsRootNode->firstChild();
         child != nullptr;
         child = child->nextSibling()) {
        labelNodes.append(static_cast<LabelTextureNode *>(child));
    }

    return labelNodes;
}

void ensureLabelNodeCount(QSGNode *labelsRootNode,
                          QVector<LabelTextureNode *> &labelNodes,
                          int requiredCount)
{
    if (!labelsRootNode)
        return;

    while (labelNodes.size() < requiredCount) {
        auto *node = new LabelTextureNode();
        node->setOwnsTexture(true);
        labelsRootNode->appendChildNode(node);
        labelNodes.append(node);
    }
}

void hideUnusedNodes(const QVector<LabelTextureNode *> &labelNodes,
                     int firstUnusedIndex)
{
    if (firstUnusedIndex < 0)
        firstUnusedIndex = 0;

    for (int i = firstUnusedIndex; i < labelNodes.size(); ++i)
        labelNodes.at(i)->setRect(QRectF(0, 0, 0, 0));
}

void clearAllLabelTextures(QSGNode *labelsRootNode)
{
    if (!labelsRootNode)
        return;

    for (QSGNode *child = labelsRootNode->firstChild();
         child != nullptr;
         child = child->nextSibling()) {
        auto *node = static_cast<LabelTextureNode *>(child);
        node->cacheKey.clear();
        node->textureBytes = 0;
        node->setTexture(nullptr);
    }
}

} // namespace LabelRenderPass
