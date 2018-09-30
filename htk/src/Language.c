/*-------------------------------------------------------- */
/*                                                         */
/*     Author : Julian Odell, Cambridge University         */
/*                              Engineering Department     */
/*                                                         */
/*---------------------------------------------------------*/
/*                                                         */
/*       File : Language.c                                 */
/*              Shell interface to the various types       */
/*              of language model.                         */
/*                                                         */
/*       $Header: /export/di/ws97/tools/srilm-0.97beta/htk/src/RCS/Language.c,v 1.4 1997/07/19 01:40:48 stolcke Exp $                                         */
/*                                                         */
/*---------------------------------------------------------*/
/*                                                         */
/*       Support for interpolated language models          */
/*       by Thomas Niesler, Cambridge University           */
/*       Engineering Department, May 1996.                 */
/*                                                         */
/*---------------------------------------------------------*/


       /* Additional comments by trn, May 1995 */

/* This is the shell module that users can modify when they */
/*  want to support many new types of language model.       */
/* Each type of language model provides a set of functions  */
/*  that are added to the shell functions below.            */
/* MyLangModel contains dummy (IE empty) functions that can */
/*  be used when only a single type of user LM is needed.   */

#include "include.h"

#define LANG_ERR     5000

/*  Define type number for supported language models.  */
/*   Add new ones between l_ngram and l_invalid.       */

typedef enum 
{ 
  l_ngram = 1,
  l_interpolate,
  l_mylm,          /* Default empty user definable LM */
  l_srilm,
  l_invalid
}
LanguageType;

/*  Set the text key for each supported language model.  */
/*   The first line of the LM file must match the key.   */
/*   By convention this is a single uppercase word.      */

static char *lmIdentifier[l_invalid+1] =
{
  "",              /* No 0 */
  "!NGRAM",        /* l_ngram   = 1 : NGram */
  "!INTERPOLATE",  /* l_interpolate = 2 : interpolated language models */
  "!MYLANGMODEL",  /* l_mylm    = 3 : Default name for user LM */
  "!SRILM",        /* l_srilm = 4 : SRI LM */
  ""               /* l_invalid */
};

/*  Need definitions of external functions to compile */

#include "NGram.h"
#include "Interpolate.h"
#include "MyLangModel.h"  /* External function definitions for user LM */
#include "SriLM.h"

/* EXPORT->ReadLangModel:  Called to read language model from filename. */
/*   Determines type of language model (from key in first line of file) */
/*   then hands over control to that module.                            */
/*  Support: Required.                                                  */


/* -----------------------------------------------------------------------
                              ReadLangModel

   Load a language model into memory.

   scale : grammar scale factor
   pen   : word insertion penalty
   ngram : lattice-internal ngram scaling factor (used by JAStar)
   -----------------------------------------------------------------------
*/

Language *ReadLangModel(Vocab *vocab,char *filename,
                        float scale,float pen,float ngram)
{
  Language *lang;
  FILE *file;
  char buf[256],*ptr;
  int i,flags;

  lang=qalloc(sizeof(Language),1,"Language:ReadLanguage:lang");

  lang->type  = l_invalid;
  lang->vocab = vocab;
  lang->scale = scale;
  lang->pen   = pen;
  lang->name  = qalloc(1,strlen(filename)+1,"Language:ReadLanguage:lang->name");
  strcpy(lang->name,filename);

  LockSymTable(vocab->wdst);

  if ((file=fopen(filename,"r"))==NULL) 
    HError(LANG_ERR+1,"Cannot open file %s to read language",filename);

  fgets(buf,256,file);		/* Get string (max 256 chars) from file */

  for (ptr=buf;*ptr;*ptr++=toupper(*ptr)); /* Get LM identifier */
  while(isspace(*(--ptr))) *ptr=0;         /* Remove whitespaces */
  for (ptr=buf;isspace(*ptr);*ptr++=0);
  
  for (i=1;i<l_invalid;i++) {              /* Check to see which lm key the */
    if (!strcmp(ptr,lmIdentifier[i]))      /* string <ptr> corresponds to */
      lang->type=i;
  }

  if (lang->type==l_invalid)
    HError(LANG_ERR+1,"Do not recognise LM type %s",ptr);

  switch(lang->type)   /* Now call corresponding <Read> function */
    {
    case l_ngram:
      flags=ReadNGram(lang,file);
      break;
    case l_interpolate:
      flags=ReadInterpolate(lang,file);
      break;
    case l_mylm:
      flags=ReadMyLangModel(lang,file);
      break;

    /*  Add new LM types here  */
    /*  flags=Read????(lang,file); */

    case l_srilm:
      flags=ReadSriLM(lang,file);
      break;

    default:
      HError(LANG_ERR+2,"No read function for LM type %d",lang->type);
      break;
    }
  fclose(file);
  lang->flags=flags;
  return(lang);
}

/* EXPORT->LangModelProb: Provide likelihood of word following path. */
/*  Support: Required unless FullProb used instead.                  */


/* --------------------------------------------------------------------
                            LangModelProb
   --------------------------------------------------------------------
*/

float LangModelProb(Language *lang,Word *word,Path *path)
{
  float prob;

  if (word==lang->vocab->null)  /* Need this for !NULL */
     return(0.0);

  switch(lang->type)   /* Call prob function for correct lm */
    {
    case l_ngram:
      prob=NGramProb(lang,word,path);
      break;
    case l_mylm:
      prob=MyLangModelProb(lang,word,path);
      break;
    case l_interpolate:
      prob = InterpolateProb(lang,word,path);
      break;

    /*  Add new LM types here  */
    /*  prob=????Prob(lang,word,path); */

    case l_srilm:
      prob=SriLMProb(lang,word,path);
      break;

    default:
      HError(LANG_ERR+4,"No Prob function for LM type %d",lang->type);
    }
  return(prob);
}

/* --------------------------------------------------------------------
                           LangModelLinkTotProb
   --------------------------------------------------------------------
*/

float LangModelLinkTotProb(Language *lang,LatLink *ll, Path *path)
{
  Word *word;
  float lmprob,prob;

  if (ll->sym->user==lang->vocab->null)  /* Need this for !NULL */
     return(ll->acprob+ll->ngprob+ll->prprob);

  switch(lang->type)   /* Call linktotprob function for correct lm */
    {
     case l_interpolate:
       /* Interpolate needs to modify way terms are summed */
       prob=InterpolateLinkTotProb(lang,ll,path);
       break;

    /*  Add new LM types here  */
    /*  prob=????LinkTotProb(lang,ll,path); */

     case l_ngram:
     case l_mylm:
     case l_srilm:
       /* Could produce function of following form but not  */
       /*  really needed as support Prob operation directly */
       word=(Word*)ll->sym->user;
       lmprob=LangModelProb(lang,word,path);
       prob=ll->acprob+ll->ngprob+ll->prprob+lmprob+lang->pen;
       /* break; */
     default:
       HError(LANG_ERR+12,"No LinkTotProb function for LM type %d",lang->type);
    }
  return(prob);
}


/* EXPORT->LangModelFullProb: Provide likelihood of complete path. */
/*  Support: Optional unless used instead of Prob.                 */


/* --------------------------------------------------------------------
                            LangModelFullProb

   Returns logprob of path due to external LM (excludes acoustic, 
   penalty and internal ngram scores).
   --------------------------------------------------------------------
*/


float LangModelFullProb(Language *lang,Path *path)
{
  float prob=0.0;

  switch(lang->type)
    {
    case l_ngram:
      break;
    case l_interpolate:
      prob = InterpolateFullProb(lang,path);
      break;
    case l_mylm:
      prob = MyLangModelFullProb(lang,path);
      break;

    /*  Add new LM types here  */
    /*  prob=????FullProb(lang,path);  */

    case l_srilm:
      prob=SriLMFullProb(lang,path);
      break;

    default:
      HError(LANG_ERR+5,"No FullProb function for LM type %d",lang->type);
    }
  return(prob);
}


/* --------------------------------------------------------------------
                           LangModelFullTotProb

   Returns total logprob of path (i.e. including acoustic, penalty, 
   internal ngram and external LM scores).

   This function is needed for interpolated model support only to allow
   interpolation with the lattice-internal LM.
   --------------------------------------------------------------------
*/


float LangModelFullTotProb(Language *lang, Path *path)
{
  Path *th;
  float prob=0.0;

  switch(lang->type)
     {
      case l_interpolate:
      prob = InterpolateFullTotProb(lang,path);
      break;
      
      /*  Add new LM types here  */
      /*  prob=????FullTotProb(lang,path);  */
      
      case l_ngram:
      case l_mylm:
      case l_srilm:
      /* Could produce function of following form but not  */
      /* really needed as support FullProb operation directly */
      prob=LangModelFullProb(lang,path);
      for (th=path;th!=NULL;th=th->prev)
         prob+=th->ll->acprob+th->ll->ngprob+th->ll->prprob+lang->pen;
      /* break; */
    default:
      HError(LANG_ERR+13,"No FullTotProb function for LM type %d",lang->type);
     }
  return(prob);
}


/* EXPORT->LangModelRePtr: Provide recombination pointer for path.   */
/*   Paths for which identical continuations will have identical     */
/*   likelihoods should give same RePtr value.  NULL == no equiv.    */
/*  Support: Optional:  Lattice rebuild requires recombination info. */

/* --------------------------------------------------------------------
                             LangModelRePtr
   --------------------------------------------------------------------
*/


void *LangModelRePtr(Language *lang,Path *path)
{
  void *re;

  switch(lang->type)
    {
    case l_ngram:
      re=NGramRePtr(lang,path);
      break;
    case l_interpolate:
      re=InterpolateRePtr(lang,path);
      break;
    case l_mylm:
      re=MyLangModelRePtr(lang,path);
      break;

    /*  Add new LM types here  */
    /*  re=????RePtr(lang,path); */

    case l_srilm:
      re=SriLMRePtr(lang,path);
      break;

    default:
      HError(LANG_ERR+6,"No RePtr function for LM type %d",lang->type);
    }
  return(re);
}


/* --------------------------------------------------------------------------
                            LAheadLangModel

   Return lattice with link lookahead fields set to LM lookahead values. 

   --------------------------------------------------------------------------
*/

Lattice *LAheadLangModel(Language *lang,Lattice *lat)
{
  Lattice  *new = NULL;

  /* Get rebuilt lattice with LM scores on each link */

  switch(lang->type) 
    {
     case l_ngram:
       new = LAheadNGram(lang,lat); 
       break;
     case l_interpolate:
       new = LAheadInterpolate(lang,lat);
       break;
    case l_mylm:
      break;

    /*  Add new LM types here  */
    /*  new=LAhead????(lang,lat); */

    default:
       HError(LANG_ERR+14,"No LAhead???? function for LM type %d",lang->type);
      break;
    }

   return (new);
}



/* EXPORT->: ApLatLangModel */

/* ----------------------------------------------------------------------
                               ApLatLangModel

   Set lookahead fields in lattice for currenly installed language model.
   ----------------------------------------------------------------------
*/


Lattice *ApLatLangModel(Language *lang,Lattice *lat)
{
  Lattice *new=NULL;
  
  switch(lang->type)
    {
    case l_ngram:
      new=ApLatNGram(lang,lat);
      break;
    case l_interpolate:
      new=ApLatInterpolate(lang,lat);
      break;
    case l_mylm:
      new=ApLatMyLangModel(lang,lat);
      break;

    /*  Add new LM types here  */
    /*  new=ApLat????(lang,lat); */

    case l_srilm:
      new=ApLatSriLM(lang,lat);
      break;

    default:
      HError(LANG_ERR+7,"No ApLat function for LM type %d",lang->type);
      break;
    }
  return(new);
}

/* EXPORT->StartLangModel: Initialise language model for new file.  */
/*  Support: Optional, available for use by sentence dependent LMs. */
/*         optional lattice can be rebuild for JAStar NBest output. */
 

/* --------------------------------------------------------------------
                              StartLangModel
   --------------------------------------------------------------------
*/


void StartLangModel(Language *lang,char *datafn)
{
  switch(lang->type)
    {
    case l_ngram:
      break;                    /* No start-of-sentence init req for ngram */
    case l_interpolate:
      StartInterpolate(lang,datafn);
      break;
    case l_mylm:
      StartMyLangModel(lang,datafn);
      break;

    /*  Add new LM types here  */
    /*  Start????(lang,datafn); */

    case l_srilm:
      StartSriLM(lang,datafn);
      break;

    default:
      HError(LANG_ERR+8,"No Start function for LM type %d",lang->type);
      break;
    }
}

/* EXPORT->EndLangModel: Terminate language model for file.  */
/*  Support: Optional, available for sentence dependent LMs. */

/* --------------------------------------------------------------------
                               EndLangModel
    --------------------------------------------------------------------
*/


void EndLangModel(Language *lang)
{
  switch(lang->type)
    {
    case l_ngram:
      break;
    case l_interpolate:
      EndInterpolate(lang);
      break;
    case l_mylm:
      EndMyLangModel(lang);
      break;

    /*  Add new LM types here  */
    /*  End????(lang); */

    case l_srilm:
      EndSriLM(lang);
      break;

    default:
      HError(LANG_ERR+9,"No End function for LM type %d",lang->type);
      break;
    }
}

/* EXPORT->OutputLangModel: Dump language model to file. */
/*  Support: Optional, used for debugging purposes.      */

/* --------------------------------------------------------------------
                           OutputLangModel
    --------------------------------------------------------------------
*/


void OutputLangModel(Language *lang,FILE *file,int oflags)
{
  fprintf(file,"%s\n\n",lmIdentifier[lang->type]);
  switch(lang->type)
    {
    case l_ngram:
      OutputNGram(lang,file,oflags);
      break;
    case l_interpolate:
      OutputInterpolate(lang,file,oflags);
      break;
    case l_mylm:
      OutputMyLangModel(lang,file,oflags);
      break;

    /*  Add new LM types here  */
    /*  Output????(lang,file,oflags); */

    default:
      HError(LANG_ERR+10,"No Output function for LM type %d",lang->type);
      break;
    }
}

/* EXPORT->LangModelAllProbs: Provide likelihood for continuations to path. */
/*   Returns array of likelihoods indexed by word->lid.                     */
/*  Support: Optional, only needed for full perplexity checking.            */


/* --------------------------------------------------------------------
                            LangModelAllProbs
   --------------------------------------------------------------------
*/


float *LangModelAllProbs(Language *lang,Path *path)
{
  float *probs;

  switch(lang->type)
    {
    case l_ngram:
      probs=NGramAllProbs(lang,path);
      break;
    case l_interpolate:
      probs=InterpolateAllProbs(lang,path);
      break;
    case l_mylm:
      probs=MyLangModelAllProbs(lang,path);
      break;

    /*  Add new LM types here  */
    /*  probs=????AllProbs(lang,path); */

    case l_srilm:
      probs=SriLMAllProbs(lang,path);
      break;
    default:
      HError(LANG_ERR+11,"No AllProbs function for LM type %d",lang->type);
    }
  return(probs);
}

/* ----------------------- END: Language.c -------------------------- */

