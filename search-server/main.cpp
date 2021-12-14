// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь: 100 чисел.
#include <iostream>

using namespace std;

int main(){

	cout << "How many numbers from 1 to 1000 are containing at least one digit 3?"s << endl;
	short numbers = 1000;
	//в каждый десяток содержит как минимум 1 цифру 3.
	short digit_3_count = numbers/10;
	cout << "There are 100 numbers, from 1 to 1000, that are containing at least one digit 3"s << endl;

	return 0;
}
// Закомитьте изменения и отправьте их в свой репозиторий.
