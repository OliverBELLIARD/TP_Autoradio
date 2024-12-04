/*
 * functions.c
 *
 *  Created on: Dec 3, 2024
 *      Author: oliver
 */

#include <stdio.h>
#include <stdlib.h>
#include "functions.h"

#include "../drivers/MCP23S17.h"


int fonction(int argc, char ** argv)
{
	if (argc > 1)
	{
		for (int i = 0; i < argc; i++)
		{
			printf("Paramètre [%d] = %s\r\n", i+1, argv[i]);
		}
	}

	return 0;
}

int calcul(int argc, char ** argv)
{
	if (argc >= 4)
	{
		switch(argv[2][0])
		{
		case '+':
			printf("%s + %s = %d\r\n", argv[1], argv[3], atoi(argv[1])+atoi(argv[3]));
			break;
		case '-':
			printf("%s - %s = %d\r\n", argv[1], argv[3], atoi(argv[1])-atoi(argv[3]));
			break;
		case '*':
		case 'x':
			printf("%s * %s = %d\r\n", argv[1], argv[3], atoi(argv[1])*atoi(argv[3]));
			break;
		default:
			printf("Opération '%s' non supporté!\r\n", argv[2]);
		}
	}

	return 0;
}

int addition(int argc, char ** argv)
{
	if (argc > 1)
	{
		int somme = 0;
		for (int i = 1; i < argc; i++)
		{
			printf(" + %s", argv[i]);
			somme = somme + atoi(argv[i]);
		}

		printf(" = %d\r\n", somme);
	}
	return 0;
}

int GPIOExpander_toggle_LED(int argc, char ** argv)
{
	if (argc > 1)
	{
		for (int i = 1; i < argc; i++)
		{
			MCP23S17_Toggle_LED_id(atoi(argv[i]));
		}
	}

	return 0;
}

int GPIOExpander_set_LED(int argc, char ** argv)
{
	if (argc > 1)
	{
		MCP23S17_Set_LED_id(atoi(argv[1]));
	}

	return 0;
}
