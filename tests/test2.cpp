#include <iostream>

#include <bddisasm.h>
#include <SafetyHook.hpp>

__declspec(noinline) int add_42(int a) {
    return a + 42;
}

void hooked_add_42(safetyhook::Context& ctx) {
#ifdef _M_X64
    ctx.rax = 1337;
#else
    ctx.eax = 1337;
#endif
}

SafetyMidHook g_hook{};

int main(int argc, char* argv[]) {
    std::cout << add_42(2) << "\n";

    {
        // Lets disassemble add_42 and hook its RET.
        auto ip = (uintptr_t)add_42;

        while (*(uint8_t*)ip != 0xC3) {
            INSTRUX ix{};
            NdDecode(&ix, (const uint8_t*)ip, ND_CODE_64, ND_DATA_64);

            // Follow JMPs
            if (ix.OpCodeBytes[0] == 0xE9) {
                ip += ix.Length + (int32_t)ix.RelativeOffset;
            } else {
                ip += ix.Length;
            }
        }

        auto factory = SafetyHookFactory::init();
        auto builder = factory->acquire();
        g_hook = builder.create_mid((void*)ip, hooked_add_42);
    }

    std::cout << add_42(3) << "\n";

    g_hook.reset();

    std::cout << add_42(4) << "\n";

    return 0;
}
