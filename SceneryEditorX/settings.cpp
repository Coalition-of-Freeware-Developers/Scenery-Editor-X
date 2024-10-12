#include <iostream>
#include "iniparser.hpp"

int main(int argc, char** argv)
{
	int ival;
	double dval;
	// Creating ini file object
	INI::File ft;
	// Loading from file
	if (!ft.Load("settings.ini"))
	{
		// Loading from stream
		std::ifstream op("opts.ini",std::ios::in);
		if (op.is_open())
			op >> ft;
	}
	// Set "value1" in section "Main" to 20
	ft.GetSection("Main")->SetValue("value1",20);
	// get "value1" from section "Main". If there is no such entry return default value 1
	ival = ft.GetSection("Main")->GetValue("value1",1).AsInt();
	// get "value2" from section "Main", defaulting to 2
	ival = ft.GetValue("Main:value2",2).AsInt();
	// set "value1" in subsection "subsect" of section "Main"
	ft.SetValue("Main.subsect:value1",345.5);
	// get "value1" from subsection "subsect" of section "Main"
	dval = ft.GetSection("Main")->GetSubSection("subsect")->GetValue("value1").AsDouble();
	// set 4th element of array "array" in section "Main" to 32.1
	ft.GetSection("Main")->SetArrayValue("array",4,32.1);
	// get 4th element of array "array" in section "Main"
	dval = ft.GetSection("Main")->GetValue("array").AsArray()[4].AsDouble();
	// iterate over all sections in file
	for (INI::File::sections_iter it = ft.SectionsBegin(); it != ft.SectionsEnd(); ++it)
	{
		std::cout << "Section name: " << it->first << std::endl;
		INI::Section* sect = it->second;
		// iterate over all entries in specific section
		for (INI::Section::values_iter it2 = sect->ValuesBegin(); it2 != sect->ValuesEnd(); ++it2)
			std::cout << "Entry name: " << it2->first << ", Entry value: " 
					  << it2->second.AsString() << std::endl;
	}
	// output to INI file
	ft.Save("settings.ini");
	// output to stream
	std::cout << ft;
}