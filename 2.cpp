#include <stdio.h>
int main()
{
	char s[6][10]={"Danil","Olya","Slava","Ann","Nikita"};
	char ss[2000];
	while (~scanf("%s",ss))
	{
		int ans=0;
		for (int i=0;ss[i]!='\0';i++)
			for (int j=0;j<5;j++)
			{
				for (int k=0;s[j][k]!='\0';k++)
					if (ss[i+k]!=s[j][k])
						goto out;
				ans++;
				out:;
			}
		if (ans==1) puts("YES");
		else puts("NO");
	}
	return 0;	
} 
