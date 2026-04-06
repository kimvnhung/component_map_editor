#ifndef TOKENKEYFEATUREFLAGS_H
#define TOKENKEYFEATUREFLAGS_H

namespace cme::tokenkey {

// Stage 0 guardrails for token-key feature rollout.
// Both toggles default OFF for first integration pass.
class FeatureFlags
{
public:
    static bool connectionTokenKeyEnabled();
    static void setConnectionTokenKeyEnabled(bool enabled);

    static bool inspectorTokenKeySelectorEnabled();
    static void setInspectorTokenKeySelectorEnabled(bool enabled);

    static void resetDefaults();
};

} // namespace cme::tokenkey

#endif // TOKENKEYFEATUREFLAGS_H
