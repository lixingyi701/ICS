#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp()
{
	if(free_==NULL)
	{
		printf("free=null");
		assert(0);
	}
	WP* w=free_;
	free_=free_->next;
	WP*current=head;
	if(head==NULL)
	{
		w->next=head;
		head=w;
		return head;
	}
	else
	{
		if(w->NO<head->NO)
		{
			w->next=head;
			head=w;
			return head;
		}
		while(current->next!=NULL)
		{
			if(w->NO>current->NO&&w->NO<current->next->NO)
			{
				w->next=current->next;
				current->next=w;
				return w;
			}
			current=current->next;
		}
	}
	current->next=w;
	w->next=NULL;
	return w;

}
void free_wp(int no)
{
	WP* current=head;
	if(head==NULL)
	{
		printf("head=null");
		assert(0);
	}
	if(head->NO==no)
	{
		head=head->next;
	}
	else
	{
	bool find=false;
	while(current->next!=NULL)
	{
		if(current->next->NO==no)
		{
			WP *w=current->next;
			current->next=current->next->next;
			w->next=NULL;
			current=w;
			find=true;
			break;

		}
		current=current->next;
	}
		if(!find)
		{
			printf("no find no");
			assert(0);
		}
	
	}
	memset(current->expr,0,sizeof(current->expr));
	current->val=0;
	if(free_==NULL)
	{
		current->next=NULL;
		free_=current;
		return;
	}
	if(free_->NO>current->NO)
	{
		current->next=free_;
		free_=current;
		return;
	}
	WP*cur1=free_;
	while(cur1->next!=NULL)
	{
		if(cur1->NO<current->NO&&cur1->next->NO>current->NO)
		{
			current->next=cur1->next;
			cur1->next=current;
			return;
		}
		cur1=cur1->next;
	}
	cur1->next=current;
	current->next=NULL;

	
}
bool change()
{
	WP *cur=head;
	bool ischange=false;
	while(cur)
	{
		bool success;
		uint32_t val=expr(cur->expr,&success);
		if(!success)
		{
			printf("expr fail");
			continue;
		}
		if(val!=cur->val)
		{
			ischange=true;
			printf("wp%d=%schange,old=%d,new=%d\n",cur->NO,cur->expr,cur->val,val);
			cur->val=val;
		}
		cur=cur->next;
	}
	return ischange;
}
void print_wp()
{
	WP*cur=head;
	while(cur)
	{
		printf("NO=%d,expr=%s,val=%d\n",cur->NO,cur->expr,cur->val);
		cur=cur->next;

	}
	return;
}

