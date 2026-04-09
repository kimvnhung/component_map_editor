#include "InvariantChecker.h"

#include "models/GraphModel.h"

InvariantChecker::InvariantChecker(QObject *parent)
    : QObject(parent)
{}

void InvariantChecker::registerInvariant(
    const QString &name,
    std::function<bool(const GraphModel *, QString *)> check)
{
    if (name.isEmpty() || !check)
        return;

    // Replace if the name already exists.
    for (InvariantEntry &entry : m_invariants) {
        if (entry.name == name) {
            entry.check = std::move(check);
            return;
        }
    }
    m_invariants.append({name, std::move(check)});
}

void InvariantChecker::unregisterInvariant(const QString &name)
{
    m_invariants.erase(
        std::remove_if(m_invariants.begin(), m_invariants.end(),
                       [&](const InvariantEntry &e) { return e.name == name; }),
        m_invariants.end());
}

void InvariantChecker::clearInvariants()
{
    m_invariants.clear();
}

int InvariantChecker::invariantCount() const
{
    return m_invariants.size();
}

bool InvariantChecker::checkAll(const GraphModel *graph, QString *firstViolation) const
{
    if (!graph)
        return true;

    for (const InvariantEntry &entry : m_invariants) {
        QString msg;
        if (!entry.check(graph, &msg)) {
            const QString full = entry.name + QStringLiteral(": ") + msg;
            if (firstViolation)
                *firstViolation = full;
            // const_cast is safe here: invariantViolated is a notification
            // signal with no side effects on the checker's own state.
            emit const_cast<InvariantChecker *>(this)->invariantViolated(entry.name, msg);
            return false;
        }
    }
    return true;
}

QStringList InvariantChecker::allViolations(const GraphModel *graph) const
{
    if (!graph)
        return {};

    QStringList violations;
    for (const InvariantEntry &entry : m_invariants) {
        QString msg;
        if (!entry.check(graph, &msg))
            violations.append(entry.name + QStringLiteral(": ") + msg);
    }
    return violations;
}
