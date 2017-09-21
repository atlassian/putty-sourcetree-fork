/*
* askpass.c - Integration with SSH_ASKPASS for plink
*/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "putty.h"

int askpass_get_userpass_input(prompts_t *p, unsigned char *in, int inlen)
{

	char* askpass = getenv("SSH_ASKPASS");
	char* display = getenv("DISPLAY");
	size_t curr_prompt;
	size_t preambleBufSize = 2048;
	char* preamble = NULL;
	FILE* askpassout = NULL;

	// Require SSH_ASKPASS and DISPLAY to both be set before using ASKPASS
	if (!askpass || !display)
		return -1;


	/*
	* Zero all the results, in case we abort half-way through.
	*/
	{
		int i;
		for (i = 0; i < (int)p->n_prompts; i++)
			memset(p->prompts[i]->result, 0, p->prompts[i]->resultsize);
	}
	
	preamble = (char*) malloc(sizeof(char) * preambleBufSize);
	preamble[0] = '\0';

	/*
	* Preamble.
	*/
	/* We only print the `name' caption if we have to... */
	if (p->name_reqd && p->name) 
	{
		size_t l = strlen(p->name);
		strcat_s(preamble, preambleBufSize, p->name);
		if (p->name[l-1] != '\n')
			strcat_s(preamble, preambleBufSize, "\n");
	}

	/* ...but we always print any `instruction'. */
	if (p->instruction) 
	{
		size_t l = strlen(p->instruction);
		strcat_s(preamble, preambleBufSize, p->instruction);
		if (p->instruction[l-1] != '\n')
			strcat_s(preamble, preambleBufSize, "\n");
	}

	for (curr_prompt = 0; curr_prompt < p->n_prompts; curr_prompt++) 
	{
		prompt_t *pr = p->prompts[curr_prompt];

		size_t bufSize = 4096;
		char* command = (char*) malloc(sizeof(char) * bufSize);

		// We call ASKPASS program once for each prompt
		// Note that not only are the askpass exe name and the prompt parameter enclosed in quotes, 
		// but the entire string is then re-quoted in it's entirety too. Ie
		// ""path\to\askpass.exe" "prompt""
		// This is the only way to deal with spaces in the askpass path in _popen, which fails if
		// you only quote the exe path (and you can't escape spaces any other way)
		// I found this answer here: http://eli.thegreenplace.net/2011/01/28/on-spaces-in-the-paths-of-programs-and-files-on-windows/
		sprintf_s(command, bufSize, "\"\"%s\" \"%s%s\"\"", askpass, preamble, pr->prompt);

		askpassout = _popen(command, "rt");
		if (!askpassout)
		{
			free(command);
			free(preamble);
			fprintf(stderr, "Failed to call SSH_ASKPASS for prompt: %s\n", pr->prompt);
		    cleanup_exit(1);
		}

		// Read askpass program's stdout up to the limit of the space allocated by caller
		if (fgets(pr->result, pr->resultsize, askpassout) == NULL)
		{
			// All was not well
			free(command);
			free(preamble);
			fprintf(stderr, "Error reading SSH_ASKPASS output for prompt: %s\n", pr->prompt);
		    cleanup_exit(1);
		}

		// Exclude carriage return
		{
			int l = strlen(pr->result);
			if (pr->result[l-1] == '\n')
				pr->result[l-1] = '\0';
		}

		free(command);
		command = 0;

	}

	free(preamble);
	preamble = 0;


	return 1; /* success */

}
