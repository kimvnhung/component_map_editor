#ifndef LABELRENDERPASS_H
#define LABELRENDERPASS_H

#include <QVector>

class QSGNode;
class LabelTextureNode;

namespace LabelRenderPass {

QVector<LabelTextureNode *> collectLabelNodes(QSGNode *labelsRootNode);
void ensureLabelNodeCount(QSGNode *labelsRootNode,
                          QVector<LabelTextureNode *> &labelNodes,
                          int requiredCount);
void hideUnusedNodes(const QVector<LabelTextureNode *> &labelNodes,
                     int firstUnusedIndex);
void clearAllLabelTextures(QSGNode *labelsRootNode);

} // namespace LabelRenderPass

#endif // LABELRENDERPASS_H
