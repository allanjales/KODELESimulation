// Simple class to save time windows for histogram and signal saving in Garfield++
// Author: Allan Jales

class TimeWindow
{
private:
	double min = 0.0; // ns
	double max = 5.0; // ns
	unsigned int nBins = 1000;

	double step = 0.; // ns
	void UpdateStep() { this->step = (max - min) / nBins; }

public:
	TimeWindow() { UpdateStep(); }

	// Setters
	void Set(double min, double max, unsigned int nBins)
	{
		this->min = min;
		this->max = max;
		this->nBins = nBins;
		UpdateStep();
	}

	// Getters
	[[nodiscard]] double GetMin() const noexcept {return min;}
	[[nodiscard]] double GetMax() const noexcept {return max;}
	[[nodiscard]] double GetnBins() const noexcept {return nBins;}
	[[nodiscard]] double GetStep() const noexcept {return step;}
};