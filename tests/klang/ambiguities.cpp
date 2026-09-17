// Known compile failures, each selected and diagnosed independently.
#include <klang.h>

struct Source : klang::Generator { void process() override { out = 1; } };
struct Stage : klang::Modifier { void set(klang::param, klang::param) override {} };

#ifdef INLINE_ROUTE
void probe() {
    Source source;
    Stage stage;
    klang::signal result = source(440) >> stage;
}
#endif

#ifdef ENVELOPE_PARAMETER
void probe() {
    klang::Envelope envelope;
    Stage stage;
    stage.set(envelope, 10);
}
#endif

#ifdef SCALAR_TEMPORARY
void probe() {
    Source source;
    klang::signal result = source(440) * .5f;
}
#endif

#ifdef FUNCTION_LEFT_SCALAR
void probe() {
    klang::signal result = 1 - abs(klang::signal(-.5));
}
#endif

#ifdef BANK_MONO
struct BankModel : klang::Modifier {
    klang::Bank<Stage, 3> bank;
    void process() override { in >> bank >> out; }
};
BankModel bankModel;
#endif

#ifdef CONTROL_ROUTE
struct Gate : klang::Modifier {
    operator int() { return 0; }
};
void probe() {
    klang::Control control;
    Gate gate;
    control >> gate;
}
#endif
