#include <stdio.h>

void create()
{
	int r, c, flag = 1;
	FILE *f1, *f2;
	int inputvalue;
    for(int i=1; i<=2; i++) 
	{
	    printf("ENTER THE ROW & COLUMN SIZE FOR %d MATRIX: ", i);
	    scanf("%d %d", &r, &c);
        if (flag == 1)
		{
            f1=fopen("matrixA.txt","w");
            for (int i=0; i<r; i++) 
			{
                for (int j=0; j<c; j++) 
				{
                    scanf("%d", &inputvalue);
                    fprintf(f1,"%d", inputvalue );
		    		if(j != c-1)
					{
		    			fprintf(f1, " ");
					}
				}
			fprintf(f1, "\n");
			}
		}
        else
		{
			f2=fopen("matrixB.txt", "w");
            for (int i=0; i<r; i++) 
			{
                for (int j=0; j<c; j++) 
				{
                    scanf("%d", &inputvalue);
                    fprintf(f2,"%d", inputvalue );
                    if(j != c-1)
					{
					    fprintf(f2, " ");
					}
				}
                fprintf(f2, "\n");
			}
		}
        flag = 0;
	}
}

int main() 
{
    create();
    return 0;
}
