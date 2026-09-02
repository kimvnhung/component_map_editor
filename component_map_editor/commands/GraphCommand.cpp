#include "GraphCommand.h"

GraphCommand::GraphCommand(const QString &text)
    : m_text(text)
{
}

QString GraphCommand::text() const
{
    return m_text;
}

int GraphCommand::id() const
{
    return -1;
}

bool GraphCommand::mergeWith(const GraphCommand *newer)
{
    (void)newer;
    return false;
}
