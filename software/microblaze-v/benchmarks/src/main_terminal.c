#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

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

int main(void)
{
    uint32_t start_cycles, end_cycles, total_cycles;

    uart_puts("\n--- BENCHMARK DE TERMINAL (SOBRECARGA DE UART) - MICROBLAZE V ---\n");

    while (1) {
        // Margen para que se vacíe el hardware de la UART antes del test
        k_msleep(100);

        // Captura de ciclos iniciales (reloj de CPU síncrono en la FPGA)
        start_cycles = k_cycle_get_32();

        // Ráfaga masiva a bajo nivel por registro sin pasar por la capa de Zephyr
        for (int i = 0; i < 100; i++) {
            uart_puts("Iteracion [");
            uart_put_uint32(i);
            uart_puts("]: Evaluando la velocidad de transmision de la UART y el buffer en Zephyr RTOS.\n");
        }

        // Captura de ciclos finales
        end_cycles = k_cycle_get_32();

        // Cálculo de métricas temporales
        total_cycles = end_cycles - start_cycles;

        // Imprimimos el resultado de forma segura
        k_msleep(500);
        uart_puts("\n>>> FIN TEST: 100 lineas impresas. Ciclos totales: ");
        uart_put_uint32(total_cycles);
        uart_puts(" | Tiempo: ");
        uart_put_microseconds(total_cycles);
        uart_puts(" <<<\n\n");

        k_msleep(3000);
    }

    return 0;
}
