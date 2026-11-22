// generated from file 'src/eclecticmistress.dsp' by dsp2cc:
// Code generated with Faust 2.85.9 (https://faust.grame.fr)


namespace eclecticmistress {

class Dsp: public PluginLV2 {
private:
	uint32_t fSampleRate;
	double fConst0;
	double fConst1;
	double fConst2;
	double fConst3;
	double fConst4;
	double fConst5;
	double fConst6;
	FAUSTFLOAT fHslider0;
	FAUSTFLOAT	*fHslider0_;
	double fRec2[2];
	double fConst7;
	double fRec1[2];
	FAUSTFLOAT fHslider1;
	FAUSTFLOAT	*fHslider1_;
	double fRec3[2];
	FAUSTFLOAT fHslider2;
	FAUSTFLOAT	*fHslider2_;
	double fRec5[2];
	int IOTA0;
	int iConst8;
	double fConst9;
	double fConst10;
	double fConst11;
	double fConst12;
	double fConst13;
	double fConst14;
	double fConst15;
	double fRec6[3];
	double fConst16;
	double fRec4[1024];
	double fRec0[3];
	double fConst17;
	FAUSTFLOAT fHslider3;
	FAUSTFLOAT	*fHslider3_;
	double fRec7[2];

	void connect(uint32_t port,void* data);
	void clear_state_f();
	void init(uint32_t sample_rate);
	void compute(int count, FAUSTFLOAT *input0, FAUSTFLOAT *output0);

	static void clear_state_f_static(PluginLV2*);
	static void init_static(uint32_t sample_rate, PluginLV2*);
	static void compute_static(int count, FAUSTFLOAT *input0, FAUSTFLOAT *output0, PluginLV2*);
	static void del_instance(PluginLV2 *p);
	static void connect_static(uint32_t port,void* data, PluginLV2 *p);
public:
	Dsp();
	~Dsp();
};



Dsp::Dsp()
	: PluginLV2() {
	version = PLUGINLV2_VERSION;
	id = "eclecticmistress";
	name = N_("Eclectic Mistress");
	mono_audio = compute_static;
	stereo_audio = 0;
	set_samplerate = init_static;
	activate_plugin = 0;
	connect_ports = connect_static;
	clear_state = clear_state_f_static;
	delete_instance = del_instance;
}

Dsp::~Dsp() {
}

inline void Dsp::clear_state_f()
{
	for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec2[l0] = 0.0;
	for (int l1 = 0; l1 < 2; l1 = l1 + 1) fRec1[l1] = 0.0;
	for (int l2 = 0; l2 < 2; l2 = l2 + 1) fRec3[l2] = 0.0;
	for (int l3 = 0; l3 < 2; l3 = l3 + 1) fRec5[l3] = 0.0;
	for (int l4 = 0; l4 < 3; l4 = l4 + 1) fRec6[l4] = 0.0;
	for (int l5 = 0; l5 < 1024; l5 = l5 + 1) fRec4[l5] = 0.0;
	for (int l6 = 0; l6 < 3; l6 = l6 + 1) fRec0[l6] = 0.0;
	for (int l7 = 0; l7 < 2; l7 = l7 + 1) fRec7[l7] = 0.0;
}

void Dsp::clear_state_f_static(PluginLV2 *p)
{
	static_cast<Dsp*>(p)->clear_state_f();
}

inline void Dsp::init(uint32_t sample_rate)
{
	fSampleRate = sample_rate;
	fConst0 = std::min<double>(1.92e+05, std::max<double>(1.0, static_cast<double>(fSampleRate)));
	fConst1 = std::tan(18849.55592153876 / fConst0);
	fConst2 = 2.0 * (1.0 - 1.0 / mydsp_faustpower2_f(fConst1));
	fConst3 = 1.0 / fConst1;
	fConst4 = (fConst3 + -1.414213562373095) / fConst1 + 1.0;
	fConst5 = (fConst3 + 1.414213562373095) / fConst1 + 1.0;
	fConst6 = 1.0 / fConst5;
	fConst7 = 1.0 / fConst0;
	iConst8 = static_cast<int>(0.005 * fConst0) + 2;
	fConst9 = std::tan(942.4777960769379 / fConst0);
	fConst10 = mydsp_faustpower2_f(fConst9);
	fConst11 = 2.0 * (1.0 - 1.0 / fConst10);
	fConst12 = 1.0 / fConst9;
	fConst13 = (fConst12 + -1.414213562373095) / fConst9 + 1.0;
	fConst14 = (fConst12 + 1.414213562373095) / fConst9 + 1.0;
	fConst15 = 1.0 / fConst14;
	fConst16 = 1.0 / (fConst10 * fConst14);
	fConst17 = 0.5 / fConst5;
	IOTA0 = 0;
	clear_state_f();
}

void Dsp::init_static(uint32_t sample_rate, PluginLV2 *p)
{
	static_cast<Dsp*>(p)->init(sample_rate);
}

void always_inline Dsp::compute(int count, FAUSTFLOAT *input0, FAUSTFLOAT *output0)
{
#define fHslider0 (*fHslider0_)
#define fHslider1 (*fHslider1_)
#define fHslider2 (*fHslider2_)
#define fHslider3 (*fHslider3_)
	double fSlow0 = 0.0010000000000000009 * static_cast<double>(fHslider0);
	double fSlow1 = 0.0010000000000000009 * static_cast<double>(fHslider1);
	double fSlow2 = 0.0010000000000000009 * static_cast<double>(fHslider2);
	double fSlow3 = 0.0010000000000000009 * static_cast<double>(fHslider3);
	for (int i0 = 0; i0 < count; i0 = i0 + 1) {
		fRec2[0] = fSlow0 + 0.999 * fRec2[1];
		fRec1[0] = fRec1[1] + fConst7 * fRec2[0];
		fRec3[0] = fSlow1 + 0.999 * fRec3[1];
		double fTemp0 = fConst0 * (2.25e-05 * fRec3[0] * (2.0 * (1.0 - std::fabs(2.0 * (fRec1[0] - std::floor(fRec1[0])) + -1.0)) + -1.0) + 0.00275);
		double fTemp1 = std::floor(fTemp0);
		fRec5[0] = fSlow2 + 0.999 * fRec5[1];
		double fTemp2 = static_cast<double>(input0[i0]);
		fRec6[0] = fTemp2 - fConst15 * (fConst13 * fRec6[2] + fConst11 * fRec6[1]);
		fRec4[IOTA0 & 1023] = fConst16 * (fRec6[2] + (fRec6[0] - 2.0 * fRec6[1])) + fRec4[(IOTA0 - (std::min<int>(iConst8, std::max<int>(0, static_cast<int>(std::round(fTemp0)) + -1)) + 1)) & 1023] * std::max<double>(-0.99, std::min<double>(0.99, 0.01 * fRec5[0]));
		int iTemp3 = static_cast<int>(fTemp0);
		fRec0[0] = (fTemp0 - fTemp1) * fRec4[(IOTA0 - std::min<int>(iConst8, std::max<int>(0, iTemp3 + 1))) & 1023] + fRec4[IOTA0 & 1023] + fRec4[(IOTA0 - std::min<int>(iConst8, std::max<int>(0, iTemp3))) & 1023] * (fTemp1 + (1.0 - fTemp0)) - fConst6 * (fConst4 * fRec0[2] + fConst2 * fRec0[1]);
		fRec7[0] = fSlow3 + 0.999 * fRec7[1];
		output0[i0] = static_cast<FAUSTFLOAT>(fTemp2 * (1.0 - 0.01 * fRec7[0]) + 0.01 * fRec7[0] * tanh(fConst17 * (fRec0[2] + fRec0[0] + 2.0 * fRec0[1])));
		fRec2[1] = fRec2[0];
		fRec1[1] = fRec1[0];
		fRec3[1] = fRec3[0];
		fRec5[1] = fRec5[0];
		IOTA0 = IOTA0 + 1;
		fRec6[2] = fRec6[1];
		fRec6[1] = fRec6[0];
		fRec0[2] = fRec0[1];
		fRec0[1] = fRec0[0];
		fRec7[1] = fRec7[0];
	}
#undef fHslider0
#undef fHslider1
#undef fHslider2
#undef fHslider3
}

void __rt_func Dsp::compute_static(int count, FAUSTFLOAT *input0, FAUSTFLOAT *output0, PluginLV2 *p)
{
	static_cast<Dsp*>(p)->compute(count, input0, output0);
}


void Dsp::connect(uint32_t port,void* data)
{
	switch ((PortIndex)port)
	{
	case COLOR: 
		fHslider2_ = (float*)data; // , 3e+01, -1e+02, 1e+02, 1.0 
		break;
	case MIX: 
		fHslider3_ = (float*)data; // , 1e+02, 0.0, 1e+02, 1.0 
		break;
	case RANGE: 
		fHslider1_ = (float*)data; // , 3e+01, 0.0, 1e+02, 1.0 
		break;
	case RATE: 
		fHslider0_ = (float*)data; // , 0.9, 0.0, 3.0, 0.01 
		break;
	default:
		break;
	}
}

void Dsp::connect_static(uint32_t port,void* data, PluginLV2 *p)
{
	static_cast<Dsp*>(p)->connect(port, data);
}


PluginLV2 *plugin() {
	return new Dsp();
}

void Dsp::del_instance(PluginLV2 *p)
{
	delete static_cast<Dsp*>(p);
}

/*
typedef enum
{
   COLOR, 
   MIX, 
   RANGE, 
   RATE, 
} PortIndex;
*/

} // end namespace eclecticmistress
