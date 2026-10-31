#include <asm/cpu.h>
#include <def/errno.h>
#include <lib/string.h>
#include <kernel/printk.h>

static int cpuid_count(uint32_t leaf, uint32_t subleaf,
                       uint32_t *eax, uint32_t *ebx,
                       uint32_t *ecx, uint32_t *edx)
{
    uint32_t a, b, c, d;

    __asm__ volatile(
        "cpuid"
        : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
        : "0"(leaf), "2"(subleaf)
    );

    if (eax) *eax = a;
    if (ebx) *ebx = b;
    if (ecx) *ecx = c;
    if (edx) *edx = d;

    return 0;
}

void cpu_detect_caps(struct cpu *cpu){
    struct cpu_caps *caps = &cpu->caps;
    uint32_t eax, ebx, ecx, edx;
    uint32_t max_basic, max_extended;

    memset(caps, 0, sizeof(*caps));

    if (cpuid_count(0, 0, &eax, &ebx, &ecx, &edx))
        return;

    max_basic = eax;

    if (max_basic >= 1) {
        cpuid_count(1, 0, &eax, &ebx, &ecx, &edx);

        caps->has_tsc  = !!(edx & (1u << 4));
        caps->has_msr  = !!(edx & (1u << 5));
        caps->has_pae  = !!(edx & (1u << 6));
        caps->has_apic = !!(edx & (1u << 9));
        caps->has_pge  = !!(edx & (1u << 13));
        caps->has_pse  = !!(edx & (1u << 3));
        caps->has_sse  = !!(edx & (1u << 25));
        caps->has_fpu  = !!(edx & (1u << 0));
        caps->has_mmx  = !!(edx & (1u << 23));
        caps->has_xsave   = !!(ecx & (1u << 26));
        caps->has_osxsave = !!(ecx & (1u << 27));
        caps->has_avx_cpu = !!(ecx & (1u << 28));

        caps->has_avx = false;

        if (caps->has_xsave && caps->has_osxsave && caps->has_avx_cpu) {
            uint32_t xcr0_lo, xcr0_hi;

            asm volatile(
                "xgetbv"
                : "=a"(xcr0_lo), "=d"(xcr0_hi)
                : "c"(0)
            );

            /* XCR0 bit 1 = XMM; bit 2 = YMM. */
            caps->has_avx =
                (xcr0_lo & 0x6u) == 0x6u;
        }
    }

    if (cpuid_count(0x80000000u, 0,
                    &eax, &ebx, &ecx, &edx))
        return;

    max_extended = eax;

    if (max_extended >= 0x80000001u) {
        cpuid_count(0x80000001u, 0,
                    &eax, &ebx, &ecx, &edx);

        caps->has_nx = !!(edx & (1u << 20));
    }
}
