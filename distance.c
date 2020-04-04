#include<stdio.h>
#include<stdlib.h>
#include<math.h>

#define N 16

int distance(int k, int l)
{
    int result;
    result = (l - k) % N;
    if(result < 0){
        result = N + result;
    }

    return result;
}

int main(){

    int k = 5;
    int l = 9;
    int result = distance(k,l);
    printf("distancia de %d a %d = %d\n" ,k,l,result);

}