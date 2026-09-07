#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <math.h>

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
    
    // Variables volatile para evitar que el compilador optimice y elimine el bucle
    volatile float a = 12.34f;
    volatile float b = 56.78f;
    volatile float resultado = 0.0f;

    uart_puts("\n--- BENCHMARK MATEMATICO (PUNTO FLOTANTE) - MICROBLAZE V ---\n");

    while (1) {
        // Captura de ciclos iniciales
        start_cycles = k_cycle_get_32();

        // Bucle de carga matemática pesada
        for (int i = 0; i < 10000; i++) {
            resultado = (a * b) / (a + b) + sinf((float)i);
        }

        // Captura de ciclos finales
        end_cycles = k_cycle_get_32();

        // Cálculo de métricas
        total_cycles = end_cycles - start_cycles;

        // Imprimimos el resultado usando la UART mapeada del PS
        uart_puts("10.000 operaciones FP -> Ciclos de CPU: ");
        uart_put_uint32(total_cycles);
        uart_puts(" | Tiempo: ");
        uart_put_microseconds(total_cycles);
        uart_puts("\n");

        // Evitamos el warning de variable no usada
        (void)resultado; 

        k_msleep(2000);
    }

    return 0;
}
