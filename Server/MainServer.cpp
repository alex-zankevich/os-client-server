#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream> 
#include <set>
#include <map>
#include <conio.h>
#include <set>
#include <windows.h>

const int default_name_length = 150;

#define EVENT_COUNT 3

class Student{
private:
	int num;
	char name[150];
	double grade;
public:
	Student(int _num, std::string _name, double _grade) :num(_num), grade(_grade){
		std::copy(_name.begin(),_name.end(),name);
		name[_name.size()] = '\0';
	}
	Student():num(0),grade(0.0){
		name[0] = '\0';
	}
	int get_id(){
		return this->num;
	}
	std::string get_name(){
		return this->name;
	}
	double get_grade(){
		return this->grade;
	}
	void set_name(std::string _name){
		std::copy(_name.begin(), _name.end(), name);
		name[_name.size()] = '\0';
	}
	void  set_grade(double grade){
		this->grade = grade;
	}
	std::string stud_to_string(){
		return std::to_string(num) + "," + name + "," + std::to_string(grade);
	}
};

class Server{
protected:
	std::map<int,int> hash;
	int stud_size;
	std::string bin_file_path;
	std::fstream bin_stream;
	char lpszClientName[default_name_length];
	/**/
	HANDLE hWritePipe, hReadPipe;
	HANDLE hMutex;
	char* lpszMutex = "MutexName";
	HANDLE hReadModifyRequestEvents[3];
	HANDLE hReadModifyResponseEvents[3];
	char* lpszEnableReadRequest = "EnableReadRequset";
	char* lpszEnableModifyRequest = "EnableModifyRequest";
	char* lpszEnableReadResponse = "EnableReadResponse";
	char* lpszEnableModifyResponse = "EnableModifyResponse";
	char* lpszCheckAccessRequest = "CheckAccessRequest";
	char* lpszCheckAccessResponse = "CheckAccessResponse";
public:
	Server(std::string _bin_file_path) : bin_file_path(_bin_file_path){
		stud_size = sizeof(Student);
		bin_stream = std::fstream(bin_file_path, std::ios::in|std::ios::out|std::ios::binary);

		hMutex = CreateMutex(NULL, FALSE, lpszMutex);

		hReadModifyRequestEvents[0] = CreateEvent(NULL, FALSE, FALSE, lpszEnableReadRequest);
		hReadModifyRequestEvents[1] = CreateEvent(NULL, FALSE, FALSE, lpszEnableModifyRequest);
		hReadModifyRequestEvents[2] = CreateEvent(NULL, FALSE, FALSE, lpszCheckAccessRequest);

		hReadModifyResponseEvents[0] = CreateEvent(NULL, FALSE, FALSE, lpszEnableReadResponse);
		hReadModifyResponseEvents[1] = CreateEvent(NULL, FALSE, FALSE, lpszEnableModifyResponse);
		hReadModifyResponseEvents[2] = CreateEvent(NULL, FALSE, FALSE, lpszCheckAccessResponse);
		

		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;

		CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
	}
	void init_base(std::string init_file_path){
		std::ifstream fin(init_file_path);
		std::string tmp;
		int count = 0;
		Student stud_tmp;
		while (std::getline(fin, tmp)){
			stud_tmp = parse_str_to_stud(tmp);
			hash.insert(std::pair<int,int>(stud_tmp.get_id(),count++));
			bin_stream.write((char*)&stud_tmp, stud_size);
		}
		fin.close();
	}
	Student parse_str_to_stud(std::string stud_str){
		std::istringstream ss(stud_str);
		std::string token;
		std::vector<std::string> tokens;
		while (std::getline(ss, token, ',')) {
			tokens.push_back(token);
		}
		Student stud(std::stoi(tokens[0]), tokens[1], std::stod(tokens[2]));
		return stud;
	}
	void show_binary_file(){
		Student tmp;
		bin_stream.seekg(0, std::ios::beg);
		std::cout << "Storage : \n";
		for (int i = 0; i < hash.size(); i++){
			bin_stream.read((char*)&tmp, sizeof(Student));
			std::cout << tmp.stud_to_string() << std::endl;
		}
	}
	bool modify_one(int id, std::string new_name, double new_grade){
		if (hash.count(id)){
			int index = hash.at(id)*stud_size;
			bin_stream.seekg(index, std::ios::beg);
			Student tmp(id,new_name,new_grade);
			bin_stream.write((char*)&tmp, stud_size);
			return true;
		}
		else{
			return false;
		}
	}
	bool read_one(int id, Student& stud){
		if (hash.count(id)){
			int index = hash.at(id)*stud_size;
			bin_stream.seekg(index, std::ios::beg);
			bin_stream.read((char*)&stud, stud_size);
			return true;
		}
		else{
			return false;
		}
	}
	void init_clients(int amount){
		STARTUPINFO si;
		PROCESS_INFORMATION piApp;
		ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		wsprintf(lpszClientName, "%s %d %d", "..\\..\\ClientServerApp\\Debug\\Client.exe", (int)hWritePipe, (int)hReadPipe);
		for (int i = 0; i < amount; i++){
			if (!CreateProcess(NULL, lpszClientName, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &piApp)) return;
		}
	}
	void listen(){
		DWORD dwWaitResult;
		DWORD dwBytesWritten;
		DWORD dwBytesRead;
		int id = 0;
		std::set<int> mod_id;
		while (true){
			dwWaitResult = WaitForMultipleObjects(3, hReadModifyRequestEvents, FALSE, INFINITE);
			if (dwWaitResult == WAIT_OBJECT_0){
				Student temp;
				ReadFile(hReadPipe, &id, sizeof(id), &dwBytesRead, NULL);
				std::cout << "Reading studen with id : " << id << std::endl;
				read_one(id, temp);
				WriteFile(hWritePipe, &temp, sizeof(temp), &dwBytesWritten, NULL);
				SetEvent(hReadModifyResponseEvents[0]);
			}
			else if (dwWaitResult == WAIT_OBJECT_0 + 1){
				Student temp;
				ReadFile(hReadPipe, &temp, sizeof(temp), &dwBytesRead, NULL);
				std::cout << "Modifying studen with id : " << id << std::endl;
				modify_one(temp.get_id(), temp.get_name(), temp.get_grade());
				mod_id.erase(id);
				SetEvent(hReadModifyResponseEvents[1]);
				std::cout << "Modified binary file : \n";
				show_binary_file();
			}
			else if (dwWaitResult == WAIT_OBJECT_0 + 2){
				Student temp;
				ReadFile(hReadPipe, &id, sizeof(id), &dwBytesRead, NULL);
				std::cout << "Checking student with id : " << id << std::endl;
				unsigned int response = 0;
				if (!hash.count(id)){
					response = 2;
					WriteFile(hWritePipe, &response, sizeof(unsigned int), &dwBytesWritten, NULL);
				}
				else{
					if (mod_id.count(id)){
						WriteFile(hWritePipe, &response, sizeof(unsigned int), &dwBytesWritten, NULL);
					}
					else{
						response = 1;
						mod_id.insert(id);
						WriteFile(hWritePipe, &response, sizeof(unsigned int), &dwBytesWritten, NULL);
					}
				}
				SetEvent(hReadModifyResponseEvents[2]);
			}
		}
	}
};

int main(int argc, int* argv){
	Server server("studentdatabase.bin");
	server.init_base("inputdata.txt");
	server.show_binary_file();
	server.init_clients(2);
	server.listen();
	return 0;
}