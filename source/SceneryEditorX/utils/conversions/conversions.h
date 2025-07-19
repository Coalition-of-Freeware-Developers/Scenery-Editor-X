/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* conversions.h
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#pragma once

/// --------------------------------------------

namespace SceneryEditorX::Utils::Convert
{
	/// Enumerate the various units of pressure that are used
	enum class PressureUnits : uint8_t
	{
	    None,
	    KPa,
	    Bar,
	    PSI,
	    InHg,
	    KgfpCm2
	};

    class UnitConversions
    {
    public:

        GLOBAL double MetersToFeet(double meters);
        GLOBAL double FeetToMeters(double feet);

        GLOBAL double MetersToInches(double meters);
        GLOBAL double InchesToMeters(double inches);

        GLOBAL double FeetToInches(double feet);
        GLOBAL double InchesToFeet(double inches);

        GLOBAL double MetersToRadians(double meters);
        GLOBAL double RadiansToMeters(double radians);
        GLOBAL double FeetToRadians(double feet);
        GLOBAL double RadiansToFeet(double radians);
        GLOBAL double InchesToRadians(double inches);
        GLOBAL double RadiansToInches(double radians);
    };

    ///*******************************
    /// Distance Conversions
    ///*******************************

    /// Convert (statute or land) Miles to Meters
    GLOBAL float FromMi(float Miles);

    /// Convert Meters to (statute or land) Miles
    GLOBAL float ToMi(float Meters);

    /// Convert KiloMeters to Meters
    GLOBAL float FromKiloM(float Miles);

    /// Convert Meters to KiloMeters
    GLOBAL float ToKiloM(float Meters);

    /// Convert yards to Meters
    GLOBAL float FromYd(float Yards);

    /// Convert Meters to yards
    GLOBAL float ToYd(float Meters);

    /// Convert Feet to Meters
    GLOBAL float FromFt(float Feet);

    /// Convert Meters to Feet
    GLOBAL float ToFt(float Meters);

    /// Convert Inches to Meters
    GLOBAL float FromIn(float Inches);

    /// Convert Meters to Inches
    GLOBAL float ToIn(float Meters);

    /// Convert from Meters into Kilometers or Miles depending on isMetric flag
    GLOBAL float FromM(float distance, bool isMetric);

    /// Convert to Meters from Kilometers or Miles depending on isMetric flag
    GLOBAL float ToM(float distance, bool isMetric);


    ///*******************************
    /// Area Conversions Meters^2
    ///*******************************

    /// Convert from meters squared to feet squared
    GLOBAL float FromFt2(float Feet2);

    /// Convert from feet squared to meters squared
    GLOBAL float ToFt2(float Meters2);

    /// Convert from inches squared to meters squared
    GLOBAL float FromIn2(float Inches2);

    /// Convert from meters squared to inches squared
    GLOBAL float ToIn2(float Meters2);


    ///*******************************
    /// Area Conversions Meters^3
    ///*******************************

    /// Convert from cubic feet to cubic meters
    GLOBAL float FromFt3(float Feet3);

    /// Convert from cubic metres to cubic feet
    GLOBAL float ToFt3(float Meters3);

    /// Convert from cubic inches to cubic metres
    GLOBAL float FromIn3(float Inches3);

    /// Convert from cubic metres to cubic inches
    GLOBAL float ToIn3(float Meters3);


    ///*******************************
    /// Speed Conversions
    ///*******************************

    /// Convert Miles-Per-Hour to Meters-Per-Second
    GLOBAL float FromMpH(float MilesPerHour);

    /// Convert Meters-Per-Second to Miles-Per-Hour
    GLOBAL float ToMpH(float MeterPerSecond);

    /// Convert Kilometers-Per-Hour to Meters-Per-Second
    GLOBAL float FromKpH(float KiloMeterPerHour);

    /// Convert Meters-Per-Second to Kilometers-Per-Hour
    GLOBAL float ToKpH(float MeterPerSecond);

    /// Convert from Meters-Per-Second into Kilometers-Per-Hour or Miles-Per-Hour on isMetric flag
    GLOBAL float FromMpS(float speed, bool isMetric);

    /// Convert to Meters-Per-Second from Kilometers-Per-Hour or Miles-Per-Hour on isMetric flag
    GLOBAL float ToMpS(float speed, bool isMetric);


    ///*******************************
    /// Mass Conversions
    ///*******************************

    /// Convert from Pounds to Kilograms
    GLOBAL float FromLb(float lb);

    /// Convert from Kilograms to Pounds
    GLOBAL float ToLb(float kg);

    /// Convert from US Tons to Kilograms
    GLOBAL float FromTUS(float TonsUS);

    /// Convert from Kilograms to US Tons
    GLOBAL float ToTUS(float kg);

    /// Convert from UK Tons to Kilograms
    GLOBAL float FromTUK(float TonsUK);

    /// Convert from Kilograms to UK Tons
    GLOBAL float ToTUK(float kg);

    /// Convert from Kilograms to Metric Tons
    GLOBAL float Totonne(float kg);

    /// Convert from Metric Tons to Kilograms
    GLOBAL float Fromtonne(float tonne);


    ///*******************************
    /// Force Conversions
    ///*******************************

    /// Convert from Pounds of Force to Newtons
    GLOBAL float FromLbf(float lbf);

    /// Convert from Newtons to Pounds of Force
    GLOBAL float ToLbf(float newton);


    ///*******************************
    /// Flow Rate Conversions
    ///*******************************

    /// Convert from Pounds per Hour to Kilograms per Second
    GLOBAL float FromLbpH(float poundsPerHour);

    /// Convert from Kilograms per Second to Pounds per Hour
    GLOBAL float ToLbpH(float KilogramsPerSecond);


    ///*******************************
    /// Energy Conversions
    ///*******************************

    /// Convert from KiloJoules to Joules
    GLOBAL float FromKJ(float KiloJoules);

    /// Convert from Joules to KiloJoules
    GLOBAL float ToKJ(float joules);


    ///*******************************
    /// Power Conversions
    ///*******************************

    /// Convert from KiloWatts to Watts
    GLOBAL float FromKW(float KiloWatts);

    /// Convert from Watts to KiloWatts
    GLOBAL float ToKW(float watts);

    /// Convert from Horsepower to Watts
    GLOBAL float FromHp(float horsePowers);

    /// Convert from Watts to Horsepower
    GLOBAL float ToHp(float watts);

    /// Convert from Brake Horsepower to Watts
    GLOBAL float FromBhp(float horsePowers);

    /// Convert from Watts to Brake Horsepower
    GLOBAL float ToBhp(float watts);

    /// Convert from BTU per Second to Watts
    GLOBAL float FromBTUpS(float btuPerSecond);

    /// Convert from Watts to BTU per Second
    GLOBAL float ToBTUpS(float watts);


    ///*******************************
    /// Pressure Conversions
    ///*******************************

    /// Convert from Pounds per Square Inch to KiloPascals
    GLOBAL float FromPSI(float psi);

    /// Convert from KiloPascals to Pounds per Square Inch
    GLOBAL float ToPSI(float KiloPascal);

    /// Convert from Inches of Mercury to KiloPascals
    GLOBAL float FromInHg(float InchesMercury);

    /// Convert from KiloPascals to Inches of Mercury
    GLOBAL float ToInHg(float KiloPascal);

    /// Convert from Bars to KiloPascals
    GLOBAL float FromBar(float bar);

    /// Convert from KiloPascals to Bars
    GLOBAL float ToBar(float KiloPascal);

    /// Convert from Kilograms-force per square centimeter to KiloPascals
    GLOBAL float FromKgfpCm2(float f);

    /// Convert from KiloPascals to Kilograms-force per square centimeter
    GLOBAL float ToKgfpCm2(float KiloPascal);

    /// Convert from KiloPascals to the specified pressure unit
    GLOBAL float FromKPa(float pressure, UnitConversions outputUnit);

    /// Convert to KiloPascals from the specified pressure unit
    GLOBAL float ToKPa(float pressure, UnitConversions inputUnit);


    ///*******************************
    /// Temperature Conversions
    ///*******************************

    /// Convert from Celsius to Fahrenheit
    /*
    GLOBAL float FromBar(float pressure)
    {
        return pressure * 100.0f; // 1 Bar = 100 KPa
    }
    */

    // Convert from Inches of Mercury to KiloPascals
    /*
    GLOBAL float FromInHg(float pressure)
    {
        return pressure * 3.38639f; // 1 Inch of Mercury = 3.38639 KPa
    }
    */

    // Convert from Kilograms-force per square centimeter to KiloPascals
    /*
    GLOBAL float FromKgfpCm2(float pressure)
    {
        return pressure * 98.0665f; // 1 Kg/cm^2 = 98.0665 KPa
    }
    */

    // Convert from Pounds per Square Inch to KiloPascals
    /*
    GLOBAL float FromPSI(float pressure)
    {
        return pressure * 6.89476f; // 1 PSI = 6.89476 KPa
    }

    };
	*/

    ///*******************************
    /// Distance Conversions
    ///*******************************
    namespace ConvMeters2
    {
		/// Convert from Feet squared to Meters squared
        GLOBAL float FromFt2(float Feet2);

		/// Convert from cubic Meters to cubic Feet
        GLOBAL float ToFt2(float Meters2);

		/// Convert from cubic Inches to cubic Meters
        GLOBAL float FromIn2(float Inches2);

		/// Convert from cubic Meters to cubic Inches
		GLOBAL float ToIn2(float Meters2);

    }; // namespace ConvMeters2

    ///*******************************
    /// Area Conversions Meters^3
    ///*******************************
    namespace ConvMeters3
    {

		/// Convert from cubic Feet to cubic Meters
		GLOBAL float FromFt3(float Feet3);

		/// Convert from cubic Meters to cubic Feet
		GLOBAL float ToFt3(float Meters3);

		/// Convert from cubic Inches to cubic Meters
		GLOBAL float FromIn3(float Inches3);

		/// Convert from cubic Meters to cubic Inches
		GLOBAL float ToIn3(float Meters3);

    }; // namespace ConvMeters3


    ///*******************************
    /// Speed Conversions
    ///*******************************
    namespace ConvMpS
    {
		/// Convert from Miles-Per-Hour to Meters-Per-Second
		GLOBAL float FromMpH(float MilesPerHour);

		/// Convert from Meters-Per-Second to Miles-Per-Hour
		GLOBAL float ToMpH(float MeterPerSecond);

		/// Convert from Kilometers-Per-Hour to Meters-Per-Second
		GLOBAL float FromKpH(float KiloMeterPerHour);

		/// Convert from Meters-Per-Second to Kilometers-Per-Hour
		GLOBAL float ToKpH(float MeterPerSecond);

		/// Convert from Meters-Per-Second into Kilometers-Per-Hour or Miles-Per-Hour on isMetric flag
		GLOBAL float FromMpS(float speed, bool isMetric);

		/// Convert to Meters-Per-Second from Kilometers-Per-Hour or Miles-Per-Hour on isMetric flag
		GLOBAL float ToMpS(float speed, bool isMetric);

    }; // namespace ConvMpS


    ///*******************************
    /// Mass Conversions
    ///*******************************
    namespace ConvKg
    {
		/// Convert from Pounds to Kilograms
		GLOBAL float FromLb(float lb);

		/// Convert from Kilograms to Pounds
		GLOBAL float ToLb(float kg);

		/// Convert from US Tons to Kilograms
		GLOBAL float FromTUS(float tonsUS);

		/// Convert from Kilograms to US Tons
		GLOBAL float ToTUS(float kg);

		/// Convert from UK Tons to Kilograms
		GLOBAL float FromTUK(float tonsUK);

		/// Convert from Kilograms to UK Tons
		GLOBAL float ToTUK(float kg);

		/// Convert from Kilograms to Metric Tons
		GLOBAL float Totonne(float kg);

		/// Convert from Metric Tons to Kilograms
		GLOBAL float Fromtonne(float tonne);

    }; // namespace ConvKg

    ///*******************************
    /// Force Conversions
    ///*******************************
    namespace Newton
    {
		/// Convert from Pounds of Force to Newtons
		GLOBAL float FromLbf(float lbf);

		/// Convert from Newtons to Pounds of Force
		GLOBAL float ToLbf(float newton);

    }; // namespace Newton


    ///*******************************
    /// Flow Rate Conversions
    ///*******************************
    namespace KgpS
    {
		/// Convert from Pounds per Hour to Kilograms per Second
		GLOBAL float FromLbpH(float poundsPerHour);

		/// Convert from Kilograms per Second to Pounds per Hour
		GLOBAL float ToLbpH(float KilogramsPerSecond);

    }; // namespace KgpS


    ///*******************************
    /// Energy Conversions
    ///*******************************
    namespace ConvJoule
    {
		/// Convert from KiloJoules to Joules
		GLOBAL float FromKJ(float KiloJoules);

		/// Convert from Joules to KiloJoules
		GLOBAL float ToKJ(float joules);

    }; // namespace ConvJoule


    ///*******************************
    /// Power Conversions
    ///*******************************
    namespace ConvWatt
    {
		/// Convert from KiloWatts to Watts
		GLOBAL float FromKW(float KiloWatts);

		/// Convert from Watts to KiloWatts
		GLOBAL float ToKW(float watts);

		/// Convert from Horsepower to Watts
		GLOBAL float FromHp(float horsePowers);

		/// Convert from Watts to Horsepower
		GLOBAL float ToHp(float watts);

		/// Convert from Brake Horsepower to Watts
		GLOBAL float FromBhp(float horsePowers);

		/// Convert from Watts to Brake Horsepower
		GLOBAL float ToBhp(float watts);

		/// Convert from BTU per Second to Watts
		GLOBAL float FromBTUpS(float btuPerSecond);

		/// Convert from Watts to BTU per Second
		GLOBAL float ToBTUpS(float watts);

    }; // namespace ConvWatt

    ///*******************************
    /// Pressure Conversions
    ///*******************************
    namespace KPa
    {
		/// Convert from Pounds per Square Inch to KiloPascals
		GLOBAL float FromPSI(float psi);

		/// Convert from KiloPascals to Pounds per Square Inch
		GLOBAL float ToPSI(float KiloPascal);

		/// Convert from Inches of Mercury to KiloPascals
		GLOBAL float FromInHg(float InchesMercury);

		/// Convert from KiloPascals to Inches of Mercury
		GLOBAL float ToInHg(float KiloPascal);

		/// Convert from Bars to KiloPascals1
		GLOBAL float FromBar(float bar);

		/// Convert from KiloPascals to Bars
		GLOBAL float ToBar(float KiloPascal);

		/// Convert from Kilograms-force per square centimeter to KiloPascals
		GLOBAL float FromKgfpCm2(float f);

		/// Convert from KiloPascals to Kilograms-force per square centimeter
		GLOBAL float ToKgfpCm2(float KiloPascal);

		/// Convert from KiloPascals to the specified pressure unit
		GLOBAL float FromKPa(float pressure, UnitConversions outputUnit);

		/// Convert to KiloPascals from the specified pressure unit
		GLOBAL float ToKPa(float pressure, UnitConversions inputUnit);

    }; // namespace KPa

} // namespace SceneryEditorX

/// --------------------------------------------
