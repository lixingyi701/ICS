#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_NUM,
  TK_HEX,
  TK_REG,
  TK_NEQ,
  TK_AND,
  TK_OR,
  TK_MINUS,
  TK_POINTER
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal

  {"-",'-'},
  {"\\*",'*'},
  {"/",'/'},
  {"\\(",'('},
  {"\\)",')'},
  {"0[xX][0-9A-Fa-f]+",TK_HEX},
  {"[1-9][0-9]*|0",TK_NUM},
  {"\\$(eax|ebx|ecx|edx|esp|ebp|esi|edi|eip|ax|bx|cx|dx|bp|sp|si|di|al|bl|cl|dl|bl|ah|ch|dh|bh)",TK_REG},
  {"!=",TK_NEQ},
  {"&&",TK_AND},
  {"\\|\\|",TK_OR},
  {"!",'!'}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        if(substr_len>32)
        {
            printf("length over 32");
            assert(0);
        }
        switch (rules[i].token_type) 
        {
            case TK_NOTYPE:
                break;
            case TK_NUM:
                tokens[nr_token].type=rules[i].token_type;
                strncpy(tokens[nr_token].str,substr_start,substr_len);
                tokens[nr_token].str[substr_len]='\0';
                nr_token++;
                break;
            case TK_HEX:
                tokens[nr_token].type=rules[i].token_type;
                strncpy(tokens[nr_token].str,substr_start+2,substr_len-2);
                tokens[nr_token].str[substr_len-2]='\0';
                nr_token++;
                break;
            case TK_REG:
                tokens[nr_token].type=rules[i].token_type;
                strncpy(tokens[nr_token].str,substr_start+1,substr_len-1);
                tokens[nr_token].str[substr_len-1]='\0';
                nr_token++;
                break;

            default: 
                tokens[nr_token].type=rules[i].token_type;	
                nr_token++;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p,int q)
{	
	bool fan=false;
	if(p>=q)
	{
		printf("error:p>=q,no parent");
		assert(0);
	}
	if(tokens[p].type=='('&&tokens[q].type==')')
	{
		int pair=0;
		for(int i=p+1;i<=q-1;i++)
		{
			if(tokens[i].type=='(')
			{
				pair++;
			}
			if(tokens[i].type==')')
			{
				if(pair>0)
				{
					pair--;
				}
				else if(pair==0)
				{
					fan=true;
					pair--;
					printf("p not match q\n");
				}
				else 
				{
					printf("error:no(before)");
					assert(0);
				}

			}
		}
		if(pair==0)
		{
			if(fan)
			{
				return false;
			}
			return true;
		}
		else 
		{
			printf("error:more(than)");
			assert(0);
		}
	}
	else 
	{
		return false;
	}

}

int priority(int tk)
{
	switch(tk)
	{
		case TK_OR:
			return 1;
		case TK_AND:
			return 2;
		case TK_EQ:
		case TK_NEQ:
			return 3;
		case '+':
		case '-':
			return 4;
		case '*':
		case '/':
			return 5;
		case '!':
		case TK_MINUS:
		case TK_POINTER:
			return 6;

		default:
			return 0;
	}
}

int findDominant(int p,int q)
{
	int re=-1,pro=7;
	for(int i=p;i<=q;i++)
	{
		if(tokens[i].type>=TK_NUM&&tokens[i].type<=TK_REG)
		{
			continue;
		}
		if(tokens[i].type=='(')
		{
			int pair=1;
			i++;
			while(pair!=0)
			{
				if(tokens[i].type==')')
				{
					pair--;
				}
				if(tokens[i].type=='(')
				{
					pair++;
				}
				i++;
			}
			i--;
			continue;
		}
		if(priority(tokens[i].type)<=pro)
		{
			re=i;
			pro=priority(tokens[i].type);
		}

	}
	return re;
}

uint32_t eval(int p,int q)
{
	if(p>q)
	{
		printf("error:p>q");
		assert(0);
	}
	else if(p==q)
	{
		if(tokens[p].type==TK_NUM)
		{
			int temp;
			sscanf(tokens[p].str,"%d",&temp);
			return temp;

		}
		else if(tokens[p].type==TK_HEX)
		{
			int temp;
			sscanf(tokens[p].str,"%x",&temp);
			return temp;

		}
		else if(tokens[p].type==TK_REG)
		{
			for(int i=0;i<8;i++)
			{
				if(strcmp(tokens[p].str,regsl[i])==0)
				{
					return reg_l(i);
				}
				if(strcmp(tokens[p].str,regsw[i])==0)
				{
					return reg_w(i);
				}
				if(strcmp(tokens[p].str,regsb[i])==0)
				{
					return reg_b(i);
				}
			}
		}
		else if(strcmp(tokens[p].str,"eip")==0)
		{
			return cpu.eip;
		}
		else
		{
			printf("error:p=q,but type wrong");
			assert(0);
		}

	}
	else if(check_parentheses(p,q)==true)
	{
		return eval(p+1,q-1);
	}
	else
	{
		int op=findDominant(p,q);
		if(tokens[op].type==TK_MINUS)
		{
			return -eval(p+1,q);
		}
		if(tokens[op].type==TK_POINTER)
		{
			vaddr_t addr;
			addr=eval(p+1,q);
			int re=vaddr_read(addr,4);
			return re;
		}
		if(tokens[op].type=='!')
		{
			int re=eval(p+1,q);
			if(re)
			{
				return 0;
			}
			else 
			{
				return 1;
			}
		}
		uint32_t val1=eval(p,op-1);
		uint32_t val2=eval(op+1,q);
		switch(tokens[op].type)
		{
			case '+':
			return val1+val2;
			case '-':
				return val1-val2;
			case '*':
				return val1*val2;
			case '/':
				return val1/val2;
			case TK_EQ:
				return val1==val2;
			case TK_NEQ:
				return val1!=val2;
			case TK_AND:
				return val1&&val2;
			case TK_OR:
				return val1||val2;
			default: 
				assert(0);
		}
	}
	return 0;
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  if(tokens[0].type=='-')
  {
	  tokens[0].type=TK_MINUS;
  }
  if(tokens[0].type=='*')
  {
	  tokens[0].type=TK_POINTER;
  }
  for(int i=1;i<nr_token;i++)
  {
	if(tokens[i].type=='-'&&tokens[i-1].type!=TK_NUM&&tokens[i-1].type!=TK_HEX&&tokens[i-1].type!=TK_REG&&tokens[i-1].type!=')')
	{
		tokens[i].type=TK_MINUS;
	}
	if(tokens[i].type=='*'&&tokens[i-1].type!=TK_NUM&&tokens[i-1].type!=TK_HEX&&tokens[i-1].type!=TK_REG&&tokens[i-1].type!=')')
	{
		tokens[i].type=TK_POINTER;
	}
  }

  /* TODO: Insert codes to evaluate the expression. */
  *success=true;
	
  return eval(0,nr_token-1);
}