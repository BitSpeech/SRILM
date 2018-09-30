/*-------------------------------------------------------- */
/*                                                         */
/*     Author : Andreas Stolcke, SRI International         */
/*                                                         */
/*---------------------------------------------------------*/
/*                                                         */
/*       File : SriLM.h                                    */
/*              Definition for HTK-LM/SRI-LM interface     */
/*                                                         */
/*       $Header: /export/di/ws97/tools/srilm-0.97beta/htk/src/RCS/SriLM.h,v 1.2 1995/07/21 04:42:38 stolcke Exp $                                         */
/*                                                         */
/*---------------------------------------------------------*/

int ReadSriLM(Language *lang,FILE *file);

void StartSriLM(Language *lang,char *datafn);
void EndSriLM(Language *lang);

float SriLMFullProb(Language *lang,Path *path);

float SriLMProb(Language *lang,Word *word,Path *path);
float *SriLMAllProbs(Language *lang,Path *path);

void *SriLMRePtr(Language *lang,Path *path);
Lattice *ApLatSriLM(Language *lang,Lattice *lat);

