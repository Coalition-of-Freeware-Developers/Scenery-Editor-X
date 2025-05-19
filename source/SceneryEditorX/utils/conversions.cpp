/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* conversions.cpp
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/utils/conversions.h>

/// --------------------------------------------

namespace SceneryEditorX
{
	///*******************************
	/// Distance Conversions
	///*******************************
	float FromMi(const float Miles)
	{
	    return Miles * 1609.344f;
	}/// Miles -> Meters
	float ToMi(const float Meters)
	{
	    return Meters * (1.0f / 1609.344f);
	}/// Meters -> Miles
	float FromKiloM(const float Km)
	{
	    return Km * 1000.0f;
	}/// KiloMeters -> Meters
	float ToKiloM(const float Meters)
	{
	    return Meters * (1.0f / 1000.0f);
	}/// Meters -> KiloMeters
	float FromYd(const float Yards)
	{
	    return Yards * 0.9144f;
	}/// Yards -> Meters
	float ToYd(const float Meters)
	{
	    return Meters * (1.0f / 0.9144f);
	}/// Meters -> Yards
	float FromFt(const float Feet)
	{
	    return Feet * 0.3048f;
	}/// Feet -> Meters
	float ToFt(const float Meters)
	{
	    return Meters * (1.0f / 0.3048f);
	}/// Meters -> Feet
	float FromIn(const float Inches)
	{
	    return Inches * 0.0254f;
	}/// Inches -> Meters
	float ToIn(const float Meters)
	{
	    return Meters * (1.0f / 0.0254f);
	} /// Meters -> Inches
	
	/// Convert from Meters into Kilometers or Miles depending on isMetric flag
	float FromM(const float distance, const bool isMetric)
	{
	    return isMetric ? ToKiloM(distance) : ToMi(distance);
	}
	
	/// Convert to Meters from Kilometers or Miles depending on isMetric flag
	float ToM(const float distance, const bool isMetric)
	{
	    return isMetric ? FromKiloM(distance) : FromMi(distance);
	}
	
	///*******************************
	/// Area Conversions Meters^2
	///*******************************
	float FromFt2(const float Feet2)
	{
	    return Feet2 * 0.092903f;
	} /// Meters^2 -> Feet^2
	float ToFt2(const float Meters2)
	{
	    return Meters2 * (1.0f / 0.092903f);
	} /// Feet^2 -> Meters^2
	float FromIn2(const float Inches2)
	{
	    return Inches2 * (1.0f / 1550.0031f);
	} /// Inches^2 -> Meters^2
	float ToIn2(const float Meters2)
	{
	    return Meters2 * 1550.0031f;
	} /// Meters^2 -> Inches^2
	
	///*******************************
	/// Area Conversions Meters^3
	///*******************************
	float FromFt3(const float Feet3)
	{
	    return Feet3 * (1.0f / 35.3146665722f);
	} /// Feet^3 -> Meters^3
	float ToFt3(const float Meters3)
	{
	    return Meters3 * 35.3146665722f;
	} /// Meters^3 -> Feet^3
	float FromIn3(const float Inches3)
	{
	    return Inches3 * (1.0f / 61023.7441f);
	} /// Inches^3 -> Meters^3
	float ToIn3(const float Meters3)
	{
	    return Meters3 * 61023.7441f;
	} /// Meters^3 -> Inches^3
	
	///*******************************
	/// Speed Conversions
	///*******************************
	float FromMpH(const float MilesPerHour)
	{
	    return MilesPerHour * (1.0f / 2.23693629f);
	} /// Miles-Per-Hour -> Meters-Per-Second
	float ToMpH(const float MeterPerSecond)
	{
	    return MeterPerSecond * 2.23693629f;
	} /// Meters-Per-Second -> Miles-Per-Hour 
	float FromKpH(const float KilometerPerHour)
	{
	    return KilometerPerHour * (1.0f / 3.600f);
	} /// Kilometers-Per-Hour -> Meters-Per-Second
	float ToKpH(const float MeterPerSecond)
	{
	    return MeterPerSecond * 3.600f;
	} /// Meters-Per-Second -> Kilometers-Per-Hour
	
	/// Convert from Meters-Per-Second into Kilometers-Per-Hour or Miles-Per-Hour on isMetric flag
	float FromMpS(const float speed, const bool isMetric)
	{
	    return isMetric ? ToKpH(speed) : ToMpH(speed);
	}
	
	/// Convert to Meters-Per-Second from Kilometers-Per-Hour or Miles-Per-Hour on isMetric flag
	float ToMpS(const float speed, const bool isMetric)
	{
	    return isMetric ? FromKpH(speed) : FromMpH(speed);
	}
	
	///*******************************
	/// Weight Conversions
	///*******************************
	
	/// Convert from Pounds to Kilograms
	float FromLb(const float lb)
	{
	    return lb * (1.0f / 2.20462f);
	}
	
	/// Convert from Kilograms to Pounds
	float ToLb(const float kg)
	{
	    return kg * 2.20462f;
	}
	
	/// Convert from US Tons to Kilograms
	float FromTUS(const float TonsUS)
	{
	    return TonsUS * 907.1847f;
	}
	
	/// Convert from Kilograms to US Tons
	float ToTUS(const float kg)
	{
	    return kg * (1.0f / 907.1847f);
	}
	
	/// Convert from UK Tons to Kilograms
	float FromTUK(const float TonsUK)
	{
	    return TonsUK * 1016.047f;
	}
	
	/// Convert from Kilograms to UK Tons
	float ToTUK(const float kg)
	{
	    return kg * (1.0f / 1016.047f);
	}
	
	/// Convert from Kilograms to Metric Tons
	float Totonne(const float kg)
	{
	    return kg * (1.0f / 1000.0f);
	}
	
	/// Convert from Metric Tons to Kilograms
	float Fromtonne(const float tonne)
	{
	    return tonne * 1000.0f;
	}
	
	///*******************************
	/// Force/Work Conversions
	///*******************************
	
	/// Convert from Pounds of Force to Newtons
	float FromLbf(const float lbf)
	{
	    return lbf * (1.0f / 0.224808943871f);
	}
	
	/// Convert from Newtons to Pounds of Force
	float ToLbf(const float newton)
	{
	    return newton * 0.224808943871f;
	}
	
	///*******************************
	/// Consumption Conversions
	///*******************************
	
	/// Convert from Pounds per Hour to Kilograms per Second
	float FromLbpH(const float poundsPerHour)
	{
	    return poundsPerHour * (1.0f / 7936.64144f);
	}
	
	/// Convert from Kilograms per Second to Pounds per Hour
	float ToLbpH(const float KilogramsPerSecond)
	{
	    return KilogramsPerSecond * 7936.64144f;
	}
	
	///*******************************
	/// Electricity Conversions
	///*******************************
	
	/// Convert from KiloJoules to Joules
	float FromKJ(const float KiloJoules)
	{
	    return KiloJoules * 1000.0f;
	}
	
	/// Convert from Joules to KiloJoules
	float ToKJ(const float joules)
	{
	    return joules * (1.0f / 1000.0f);
	}
	
	///*******************************
	/// Power Conversions
	///*******************************
	
	/// Convert from KiloWatts to Watts
	float FromKW(const float KiloWatts)
	{
	    return KiloWatts * 1000.0f;
	}
	
	/// Convert from Watts to KiloWatts
	float ToKW(const float watts)
	{
	    return watts * (1.0f / 1000.0f);
	}
	
	/// Convert from Horsepower to Watts
	float FromHp(const float horsePowers)
	{
	    return horsePowers * 745.699872f;
	}
	
	/// Convert from Watts to Horsepower
	float ToHp(const float watts)
	{
	    return watts * (1.0f / 745.699872f);
	}
	
	/// Convert from Brake Horsepower to Watts
	float FromBhp(const float horsePowers)
	{
	    return horsePowers * 9809.5f;
	}
	
	/// Convert from Watts to Brake Horsepower
	float ToBhp(const float watts)
	{
	    return watts * (1.0f / 9809.5f);
	}
	
	/// Convert from BTU per Second to Watts
	float FromBTUpS(const float btuPerSecond)
	{
	    return btuPerSecond * 1055.05585f;
	}
	
	/// Convert from Watts to BTU per Second
	float ToBTUpS(const float watts)
	{
	    return watts * (1.0f / 1055.05585f);
	}
	
	///*******************************
	/// Pressure Conversions
	///*******************************
	
	/// Convert from Pounds per Square Inch to KiloPascals
	float FromPSI(const float psi)
	{
	    return psi * 6.89475729f;
	}
	
	/// Convert from KiloPascals to Pounds per Square Inch
	float ToPSI(const float KiloPascal)
	{
	    return KiloPascal * (1.0f / 6.89475729f);
	}
	
	/// Convert from Inches of Mercury to KiloPascals
	float FromInHg(const float InchesMercury)
	{
	    return InchesMercury * 3.386389f;
	}
	
	/// Convert from KiloPascals to Inches of Mercury
	float ToInHg(const float KiloPascal)
	{
	    return KiloPascal * (1.0f / 3.386389f);
	}
	
	/// Convert from Bars to KiloPascals
	float FromBar(const float bar)
	{
	    return bar * 100.0f;
	}
	
	/// Convert from KiloPascals to Bars
	float ToBar(const float KiloPascal)
	{
	    return KiloPascal * (1.0f / 100.0f);
	}
	
	/// Convert from Kilograms-force per square centimeter to KiloPascals
	float FromKgfpCm2(const float f)
	{
	    return f * 98.068059f;
	}
	
	/// Convert from KiloPascals to Kilograms-force per square centimeter
	float ToKgfpCm2(const float KiloPascal)
	{
	    return KiloPascal * (1.0f / 98.068059f);
	}
	
	/// Convert from KiloPascals to the specified output unit
	float FromKPa(const float pressure, const PressureUnits outputUnit)
	{
	    switch (outputUnit)
	    {
	    case PressureUnits::KPa :
	        return pressure;
	    case PressureUnits::Bar :
	        return ToBar(pressure);
	    case PressureUnits::InHg :
	        return ToInHg(pressure);
	    case PressureUnits::KgfpCm2 :
	        return ToKgfpCm2(pressure);
	    case PressureUnits::PSI :
	        return ToPSI(pressure);
	    default :
	        throw std::out_of_range("Pressure unit not recognized");
	    }
	}
	
	/// Convert to KiloPascals from the specified input unit
	float ToKPa(const float pressure, const PressureUnits inputUnit)
	{
	    switch (inputUnit)
	    {
	    case PressureUnits::KPa :
	        return pressure;
	    case PressureUnits::Bar :
	        return FromBar(pressure); /// Converts Bar to KPa
	    case PressureUnits::InHg :
	        return FromInHg(pressure); /// Converts Inches of Mercury to KPa
	    case PressureUnits::KgfpCm2 :
	        return FromKgfpCm2(pressure); /// Converts Kilograms-force per square centimeter to KPa
	    case PressureUnits::PSI :
            return FromPSI(pressure); /// Converts Pounds per square inch to KPa
	    default :
	        throw std::out_of_range("Pressure unit not recognized");
	    }
	}
	
	///*******************************
	/// Temperature Conversions
	///*******************************
	namespace ConvMeters
	{
		/// Convert from statute or land Miles to Meters
		float FromMi(const float Miles)
		{
		    return Miles * 1609.344f;
		}
		
		/// Convert Meters to statute or land Miles
		float ToMi(const float Meters)
		{
		    return Meters * (1.0f / 1609.344f);
		}
		
		/// Convert KiloMeters to Meters
		float FromKiloM(const float km)
		{
		    return km * 1000.0f;
		}
		
		/// Convert Meters to KiloMeters
		float ToKiloM(const float Meters)
		{
		    return Meters * (1.0f / 1000.0f);
		}
		
		/// Convert yards to Meters
		float FromYd(const float yards)
		{
		    return yards * 0.9144f;
		}
		
		/// Convert Meters to yards
		float ToYd(const float Meters)
		{
		    return Meters * (1.0f / 0.9144f);
		}
		
		/// Convert Feet to Meters
		float FromFt(const float Feet)
		{
		    return Feet * 0.3048f;
		}
		
		/// Convert Meters to Feet
		float ToFt(const float Meters)
		{
		    return Meters * (1.0f / 0.3048f);
		}
		
		/// Convert Inches to Meters
		float FromIn(const float Inches)
		{
		    return Inches * 0.0254f;
		}
		
		/// Convert Meters to Inches
		float ToIn(const float Meters)
		{
		    return Meters * (1.0f / 0.0254f);
		}
		
		/// Convert from Meters into Kilometers or Miles depending on isMetric flag
		float FromM(const float distance, const bool isMetric)
		{
		    return isMetric ? ToKiloM(distance) : ToMi(distance);
		}
		
		/// Convert to Meters from Kilometers or Miles depending on isMetric flag
		float ToM(const float distance, const bool isMetric)
		{
		    return isMetric ? FromKiloM(distance) : FromMi(distance);
		}
	}
	
	///*******************************
	/// Area Conversions Meters^2
	///*******************************
	namespace ConvMeters2
	{
		/// Convert from Feet squared to Meters squared
		float FromFt2(const float Feet2)
		{
		    return Feet2 * 0.092903f;
		}
		
		/// Convert from cubic Meters to cubic Feet
		float ToFt2(const float Meters2)
		{
		    return Meters2 * (1.0f / 0.092903f);
		}
		
		/// Convert from cubic Inches to cubic Meters
		float FromIn2(const float Inches2)
		{
		    return Inches2 * (1.0f / 1550.0031f);
		}
		
		/// Convert from cubic Meters to cubic Inches
		float ToIn2(const float Meters2)
		{
		    return Meters2 * 1550.0031f;
		}
	}
	
	///*******************************
	/// Area Conversions Meters^3
	///*******************************
	namespace ConvMeters3
	{
		/// Convert from cubic Feet to cubic Meters
		float FromFt3(const float Feet3)
		{
		    return Feet3 * (1.0f / 35.3146665722f);
		}
		
		/// Convert from cubic Meters to cubic Feet
		float ToFt3(const float Meters3)
		{
		    return Meters3 * 35.3146665722f;
		}
		
		/// Convert from cubic Inches to cubic Meters
		float FromIn3(const float Inches3)
		{
		    return Inches3 * (1.0f / 61023.7441f);
		}
		
		/// Convert from cubic Meters to cubic Inches
		float ToIn3(const float Meters3)
		{
		    return Meters3 * 61023.7441f;
		}
	}
	
	///*******************************
	/// Speed Conversions
	///*******************************
	namespace ConvMpS
	{
		/// Convert Miles-Per-Hour to Meters-Per-Second
		float FromMpH(const float MilesPerHour)
		{
		    return MilesPerHour * (1.0f / 2.23693629f);
		}
		
		/// Convert Meters-Per-Second to Miles-Per-Hour
		float ToMpH(const float MeterPerSecond)
		{
		    return MeterPerSecond * 2.23693629f;
		}
		
		/// Convert Kilometers-Per-Hour to Meters-Per-Second
		float FromKpH(const float KiloMeterPerHour)
		{
		    return KiloMeterPerHour * (1.0f / 3.600f);
		}
		
		/// Convert Meters-Per-Second to Kilometers-Per-Hour
		float ToKpH(const float MeterPerSecond)
		{
		    return MeterPerSecond * 3.600f;
		}
		
		/// Convert from Meters-Per-Second into Kilometers-Per-Hour or Miles-Per-Hour on isMetric flag
		float FromMpS(const float speed, const bool isMetric)
		{
		    return isMetric ? ToKpH(speed) : ToMpH(speed);
		}
		
		/// Convert to Meters-Per-Second from Kilometers-Per-Hour or Miles-Per-Hour on isMetric flag
		float ToMpS(const float speed, const bool isMetric)
		{
		    return isMetric ? FromKpH(speed) : FromMpH(speed);
		}
	}
	
	///*******************************
	/// Weight Conversions
	///*******************************
	namespace ConvKg
	{
		/// Convert from Pounds to Kilograms
		float FromLb(const float lb)
		{
		    return lb * (1.0f / 2.20462f);
		}
		
		/// Convert from Kilograms to Pounds
		float ToLb(const float kg)
		{
		    return kg * 2.20462f;
		}
		
		/// Convert from US Tons to Kilograms
		float FromTUS(const float tonsUS)
		{
		    return tonsUS * 907.1847f;
		}
		
		/// Convert from Kilograms to US Tons
		float ToTUS(const float kg)
		{
		    return kg * (1.0f / 907.1847f);
		}
		
		/// Convert from UK Tons to Kilograms
		float FromTUK(const float tonsUK)
		{
		    return tonsUK * 1016.047f;
		}
		
		/// Convert from Kilograms to UK Tons
		float ToTUK(const float kg)
		{
		    return kg * (1.0f / 1016.047f);
		}
		
		/// Convert from Kilograms to Metric Tons
		float Totonne(const float kg)
		{
		    return kg * (1.0f / 1000.0f);
		}
		
		/// Convert from Metric Tons to Kilograms
		float Fromtonne(const float tonne)
		{
		    return tonne * 1000.0f;
		}
	}
	
	///*******************************
	/// Force Conversions
	///*******************************
	namespace Newton
	{
		/// Convert from Pounds of Force to Newtons
		float FromLbf(const float lbf)
		{
		    return lbf * (1.0f / 0.224808943871f);
		}
		
		/// Convert from Newtons to Pounds of Force
		float ToLbf(const float newton)
		{
		    return newton * 0.224808943871f;
		}
	}
	
	///*******************************
	/// Consumption Conversions
	///*******************************
	namespace KgpS
	{
		/// Convert from Pounds per Hour to Kilograms per Second
		float FromLbpH(const float poundsPerHour)
		{
		    return poundsPerHour * (1.0f / 7936.64144f);
		}
		
		/// Convert from Kilograms per Second to Pounds per Hour
		float ToLbpH(const float KilogramsPerSecond)
		{
		    return KilogramsPerSecond * 7936.64144f;
		}
	}
	
	///*******************************
	/// Energy Conversions
	///*******************************
	namespace ConvJoule
	{
		/// Convert from KiloJoules to Joules
		float FromKJ(const float KiloJoules)
		{
		    return KiloJoules * 1000.0f;
		}
		
		/// Convert from Joules to KiloJoules
		float ToKJ(const float joules)
		{
		    return joules * (1.0f / 1000.0f);
		}
	}
	
	
	///*******************************
	/// Power Conversions
	///*******************************
	namespace ConvWatt
	{
		/// Convert from KiloWatts to Watts
		float FromKW(const float KiloWatts)
		{
		    return KiloWatts * 1000.0f;
		}
		
		/// Convert from Watts to KiloWatts
		float ToKW(const float watts)
		{
		    return watts * (1.0f / 1000.0f);
		}
		
		/// Convert from Horsepower to Watts
		float FromHp(const float horsePowers)
		{
		    return horsePowers * 745.699872f;
		}
		
		/// Convert from Watts to Horsepower
		float ToHp(const float watts)
		{
		    return watts * (1.0f / 745.699872f);
		}
		
		/// Convert from Brake Horsepower to Watts
		float FromBhp(const float horsePowers)
		{
		    return horsePowers * 9809.5f;
		}
		
		/// Convert from Watts to Brake Horsepower
		float ToBhp(const float watts)
		{
		    return watts * (1.0f / 9809.5f);
		}
		
		/// Convert from BTU per Second to Watts
		float FromBTUpS(const float btuPerSecond)
		{
		    return btuPerSecond * 1055.05585f;
		}
		
		/// Convert from Watts to BTU per Second
		float ToBTUpS(const float watts)
		{
		    return watts * (1.0f / 1055.05585f);
		}
	}
	
	///*******************************
	/// Pressure Conversions
	///*******************************
	namespace KPa
	{
		/// Convert from Pounds per Square Inch to KiloPascals
		float FromPSI(const float psi)
		{
		    return psi * 6.89475729f;
		}
		
		/// Convert from KiloPascals to Pounds per Square Inch
		float ToPSI(const float KiloPascal)
		{
		    return KiloPascal * (1.0f / 6.89475729f);
		}
		
		/// Convert from Inches of Mercury to KiloPascals
		float FromInHg(const float InchesMercury)
		{
		    return InchesMercury * 3.386389f;
		}
		
		/// Convert from KiloPascals to Inches of Mercury
		float ToInHg(const float KiloPascal)
		{
		    return KiloPascal * (1.0f / 3.386389f);
		}
		
		/// Convert from Bars to KiloPascals
		float FromBar(const float bar)
		{
		    return bar * 100.0f;
		}
		
		/// Convert from KiloPascals to Bars
		float ToBar(const float KiloPascal)
		{
		    return KiloPascal * (1.0f / 100.0f);
		}
		
		/// Convert from Kilograms-force per square centimeter to KiloPascals
		float FromKgfpCm2(const float f)
		{
		    return f * 98.068059f;
		}
		
		/// Convert from KiloPascals to Kilograms-force per square centimeter
		float ToKgfpCm2(const float KiloPascal)
		{
		    return KiloPascal * (1.0f / 98.068059f);
		}
		
		/// Convert from KiloPascals to the specified output unit
		float FromKPa(const float pressure, const PressureUnits outputUnit)
		{
		    switch (outputUnit)
		    {
		    case PressureUnits::KPa :
		        return pressure;
		    case PressureUnits::Bar :
		        return ToBar(pressure);
		    case PressureUnits::InHg :
		        return ToInHg(pressure);
		    case PressureUnits::KgfpCm2 :
		        return ToKgfpCm2(pressure);
		    case PressureUnits::PSI :
		        return ToPSI(pressure);
		    default :
		        throw std::out_of_range("Pressure unit not recognized");
		    }
		}
		
		/// Convert to KiloPascals from the specified input unit
		float ToKPa(const float pressure, const PressureUnits inputUnit)
		{
		    switch (inputUnit)
		    {
		    case PressureUnits::KPa :
		        return pressure;
		    case PressureUnits::Bar :
		        return FromBar(pressure);
		    case PressureUnits::InHg :
		        return FromInHg(pressure);
		    case PressureUnits::KgfpCm2 :
		        return FromKgfpCm2(pressure);
		    case PressureUnits::PSI :
		        return FromPSI(pressure);
		    default :
		        throw std::out_of_range("Pressure unit not recognized");
		    }
		}
	} // namespace KPa
	
	
} // namespace SceneryEditorX

/// --------------------------------------------
