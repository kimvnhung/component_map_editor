#ifndef LABELTEXTURENODE_H
#define LABELTEXTURENODE_H

#include <QSGSimpleTextureNode>

class LabelTextureNode final : public QSGSimpleTextureNode
{
public:
    QString cacheKey;
    qint64 textureBytes = 0;
};

#endif // LABELTEXTURENODE_H
