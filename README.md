# TP de Synthèse – Autoradio
TP de Synthèse – Autoradio


## 2 Le GPIO Expander et le VU-Metre

### 2.1 Configuration

1. La référence du GPIO Expander est la suivante : MCP23S17-E/S0. Nous avons ajouté sa datasheet dans notre documentation.
2. C'est le SPI3 qui est utilisé dans ce cas. 
3. Sur STM32CubeIDE Les changements à effectuer sont l'allocation des pins du SPI3 utilisés par le VU-Metre.
4. ![image](https://github.com/user-attachments/assets/7670b71d-7e9e-43da-8ec4-54056be08e00)  
   (le pin VU_nRESET a été également alloué au VU-Metre)

### 2.2 Tests

Pour cette partie nous avons décidé de faire un chenillard en utilisant les 16 LEDs à notre disposition. Pour effectuer ce chenillard nous avons utilisé la connexion SPI3 pour allez contrôler notre GPIO Expander, celui-ci contrôlant directement nos LEDs.  

Voici un aperçu de la fonction que nous avons réalisé à cet effet :  

```C
void test_chenillard(int delay)
{
	int i = 0;

	for (;;)
	{
		MCP23S17_Set_LEDs(~(1 << i%8 | ((1 << i%8) << 8)));
		i++;

		vTaskDelay( delay / portTICK_PERIOD_MS );  // Délai de duree en ms
	}
}
```

### 2.3 Driver

