#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys_clock.h>

// --- Comunicación directa con la UART del PS (Zynq-7000) ---
#define UART_BASE      0xE0001000
#define UART_FIFO      (UART_BASE + 0x30)
#define UART_SR        (UART_BASE + 0x2C)
#define UART_SR_TXFULL (1 << 4)

static void uart_putc(char c)
{
    volatile uint32_t *sr   = (volatile uint32_t *)UART_SR;
    volatile uint32_t *fifo = (volatile uint32_t *)UART_FIFO;
    while (*sr & UART_SR_TXFULL);
    *fifo = c;
}

static void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}

// Función auxiliar para convertir un entero a texto (Base 10)
static void uart_put_uint32(uint32_t val)
{
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    
    if (val == 0) {
        uart_puts("0");
        return;
    }
    
    while (val > 0 && i >= 0) {
        buf[i--] = (val % 10) + '0';
        val /= 10;
    }
    uart_puts(&buf[i + 1]);
}

// Función para calcular y mostrar microsegundos basados en el reloj del sistema
static void uart_put_microseconds(uint32_t cycles)
{
    uint32_t freq_mhz = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000000; 
    
    if (freq_mhz == 0) {
        freq_mhz = 100; 
    }

    uint32_t us_enteros = cycles / freq_mhz;
    uint32_t us_decimales = cycles % freq_mhz;

    uart_put_uint32(us_enteros);
    uart_puts(".");
    
    if (us_decimales < 10) {
        uart_puts("0");
    }
    uart_put_uint32(us_decimales);
    uart_puts(" us");
}

// Vinculación con el alias 'test-pin' definido en tu archivo .overlay
static const struct gpio_dt_spec pin = GPIO_DT_SPEC_GET(DT_ALIAS(test_pin), gpios);

int main(void)
{
    uint32_t start_cycles, end_cycles, total_cycles;
    uint32_t test_count = 1;

    // Verificar que el periférico está listo en Zephyr y configurarlo como salida
    if (!device_is_ready(pin.port)) {
        uart_puts("Error: El periferico GPIO no esta listo.\r\n");
        return 0;
    }
    gpio_pin_configure_dt(&pin, GPIO_OUTPUT_ACTIVE);

    uart_puts("\r\n=== Monitor de Benchmark GPIO en bucle activo ===\r\n");

    // BUCLE INFINITO DEL EXPERIMENTO
    while (1) {
        uart_puts("Ejecucion #");
        uart_put_uint32(test_count++);
        uart_puts("...\r\n");

        // ========================================================
        // TOMA DE TIEMPOS - ENTORNO DE PRUEBA
        // ========================================================
        start_cycles = k_cycle_get_32();

        for (int i = 0; i < 10000; i++) {
            gpio_pin_set_dt(&pin, 1);
            gpio_pin_set_dt(&pin, 0);
        }

        end_cycles = k_cycle_get_32();
        // ========================================================

        total_cycles = end_cycles - start_cycles;

        // Volcado de resultados
        uart_puts("Ciclos: ");
        uart_put_uint32(total_cycles);
        uart_puts(" | Tiempo: ");
        uart_put_microseconds(total_cycles);
        uart_puts("\r\n-----------------------------------\r\n");

        // Un pequeño retardo manual por hardware (sin dormir el kernel) 
        // para no saturar el buffer de tu pantalla en el PC
        for (volatile int delay = 0; delay < 2000000; delay++) {
            __asm__ volatile("nop");
        }
    }

    return 0;
}
