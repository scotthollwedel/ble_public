#ifndef GPIO_H
#define GPIO_H
void gpio_init(void);
void gpio_power_up_cc3000(void);
void gpio_power_down_cc3000(void);
void gpio_enable_cc3000_irq(void);
void gpio_disable_cc3000_irq(void);
int gpio_get_cc3000_irq(void);
#endif
