#include <cassert>
#include <iostream>
#include <iomanip>
#include <string>

#include <tclap/CmdLine.h>
#include "alg.hpp"

#include "fast5.hpp"

using namespace std;


namespace opts
{
    using namespace TCLAP;
    string description = "Dump contents of ONT fast5 files.";
    CmdLine cmd_parser(description);
    //
    ValueArg< unsigned > float_prec("", "float-prec", "Float precision.", false, 10, "int", cmd_parser);
    SwitchArg rw_time("", "rw-time", "Add timepoints to raw data.", cmd_parser);
    SwitchArg curr_int("", "curr-int", "Dump current data encoded as int (raw samples only).", cmd_parser);
    SwitchArg time_int("", "time-int", "Dump start/length data encoded as int.", cmd_parser);
    //
    ValueArg< string > rn("", "rn", "Read name.", false, "", "Read_1015|...", cmd_parser);
    ValueArg< unsigned > st("", "st", "Strand.", false, 0, "0|1|2", cmd_parser);
    ValueArg< string > gr("", "gr", "Group name suffix.", false, "", "000|RNN_001|...", cmd_parser);
    //
    SwitchArg fq("", "fq", "Dump basecall fastq data.", cmd_parser);
    SwitchArg ev("", "ev", "Dump basecall event data.", cmd_parser);
    SwitchArg ed("", "ed", "Dump event detection data.", cmd_parser);
    SwitchArg rw("", "rw", "Dump raw samples data.", cmd_parser);
    SwitchArg id("", "id", "Dump channel/tracking id data.", cmd_parser);
    SwitchArg ls("", "ls", "List groups/read names.", cmd_parser);
    UnlabeledValueArg< string > input_fn("input", "Fast5 file.", true, "", "file", cmd_parser);
}

template < typename U, typename V >
void print_map(ostream& os, const map< U, V >& m, const string& prefix)
{
    for (const auto& p : m)
    {
        os << prefix << p.first << "=" << p.second << endl;
    }
}

unsigned time_int(double f, fast5::Channel_Id_Parameters const & channel_id_params)
{
    return f * channel_id_params.sampling_rate;
}

void real_main()
{
    fast5::File f;
    try
    {
        // open file
        f.open(opts::input_fn);
        auto channel_id_params = f.get_channel_id_params();
        //
        // list
        //
        if (opts::ls)
        {
            // rw
            {
                auto rn_l = f.get_raw_samples_read_name_list();
                for (auto const & rn : rn_l)
                {
                    cout << "rw\t" << rn << endl;
                }
            }
            // ed
            {
                auto gr_l = f.get_eventdetection_group_list();
                for (auto const & gr : gr_l)
                {
                    auto rn_l = f.get_eventdetection_read_name_list(gr);
                    cout << "ed\t" << gr << "\t" << alg::os_join(rn_l, ",") << endl;
                }
            }
            // bc
            {
                for (auto st : vector< unsigned >({ 2, 0, 1 }))
                {
                    auto gr_l = f.get_basecall_strand_group_list(st);
                    for (auto const & gr : gr_l)
                    {
                        int have_fastq = f.have_basecall_fastq(st, gr);
                        int have_events = (st == 2
                                           ? f.have_basecall_events(0, gr) and f.have_basecall_events(1, gr)
                                           : f.have_basecall_events(st, gr));
                        string link = (st == 2? f.get_basecall_group_1d(gr) : f.get_basecall_eventdetection_group(gr));
                        cout
                            << (st == 2? "bc2d" : "bc1d") << "\t"
                            << gr << "\t"
                            << st << "\t"
                            << have_fastq << "\t"
                            << have_events << "\t"
                            << (not link.empty()? link : string(".")) << endl
                            ;
                    }
                }
            }
        }
        //
        // id
        //
        if (opts::id)
        {
            cout
                << "channel_id/channel_number=" << channel_id_params.channel_number << endl
                << "channel_id/digitisation=" << channel_id_params.digitisation << endl
                << "channel_id/offset=" << channel_id_params.offset << endl
                << "channel_id/range=" << channel_id_params.range << endl
                << "channel_id/sampling_rate=" << channel_id_params.sampling_rate << endl
                ;
            if (f.have_tracking_id_params())
            {
                auto tracking_id_params = f.get_tracking_id_params();
                print_map(cout, tracking_id_params, "tracking_id/");
            }
        }
        //
        // raw samples
        //
        if (opts::rw and f.have_raw_samples(opts::rn))
        {
            auto rs_params = f.get_raw_samples_params(opts::rn);
            cout
                << "#read_id=" << rs_params.read_id << endl
                << "#read_number=" << rs_params.read_number << endl
                << "#start_mux=" << rs_params.start_mux << endl
                << "#start_time=" << rs_params.start_time << endl
                << "#duration=" << rs_params.duration << endl
                ;
            if (not opts::curr_int)
            {
                auto rs = f.get_raw_samples(opts::rn);
                if (opts::rw_time)
                {
                    cout << "start\t";
                }
                cout << "curr_f" << endl;
                for (unsigned i = 0; i < rs.size(); ++i)
                {
                    if (opts::rw_time)
                    {
                        cout << rs_params.start_time + i << "\t";
                    }
                    cout << rs[i] << endl;
                }
            }
            else
            {
                auto rs_int = f.get_raw_samples_int(opts::rn);
                if (opts::rw_time)
                {
                    cout << "start\t";
                }
                cout << "curr_i" << endl;
                for (unsigned i = 0; i < rs_int.size(); ++i)
                {
                    if (opts::rw_time)
                    {
                        cout << rs_params.start_time + i << "\t";
                    }
                    cout << rs_int[i] << endl;
                }
            }
        } // if opts::rw
        //
        // event detection
        //
        if (opts::ed and f.have_eventdetection_events(opts::gr, opts::rn))
        {
            auto ede_params = f.get_eventdetection_event_params(opts::gr, opts::rn);
            cout
                << "#read_id=" << ede_params.read_id << endl
                << "#read_number=" << ede_params.read_number << endl
                << "#start_mux=" << ede_params.start_mux << endl
                << "#start_time=" << ede_params.start_time << endl
                << "#duration=" << ede_params.duration << endl
                << "#scaling_used=" << ede_params.scaling_used << endl
                ;
            auto ede = f.get_eventdetection_events(opts::gr, opts::rn);
            cout
                << "start\tlength\tmean\tstdv" << endl
                << alg::os_join(ede, "\n", [] (fast5::EventDetection_Event_Entry const & e) {
                        ostringstream oss;
                        oss.precision(opts::float_prec);
                        oss << e.start << "\t" << e.length << "\t" << e.mean << "\t" << e.stdv;
                        return oss.str();
                    })
                << endl;
        } // if opts::ed
        //
        // basecall events
        //
        if (opts::ev and f.have_basecall_events(opts::st, opts::gr))
        {
            auto bce = f.get_basecall_events(opts::st, opts::gr);
            cout
                << alg::os_join(bce, "\n", [&channel_id_params] (fast5::Event_Entry const & e) {
                        ostringstream oss;
                        oss.precision(opts::float_prec);
                        if (not opts::time_int)
                        {
                            oss << e.start << "\t" << e.length << "\t";
                        }
                        else
                        {
                            oss
                                << time_int(e.start, channel_id_params) << "\t"
                                << time_int(e.length, channel_id_params) << "\t";
                        }
                        oss
                            << e.mean << "\t"
                            << e.stdv << "\t"
                            << string(e.model_state.begin(), e.model_state.end()).data() << "\t"
                            << e.move;
                        return oss.str();
                    })
                << endl;
        } // if opts::ev
        //
        // basecall fastq
        //
        if (opts::fq and f.have_basecall_fastq(opts::st, opts::gr))
        {
            auto fq = f.get_basecall_fastq(opts::st, opts::gr);
            cout << fq << endl;
        } // if opts::fq
    }
    catch (hdf5_tools::Exception& e)
    {
        cerr << opts::input_fn.get() << ": HDF5 error: " << e.what() << endl;
        exit(EXIT_FAILURE);
    }
    f.close();
} // real_main()

int main(int argc, char * argv[])
{
    opts::cmd_parser.parse(argc, argv);
    //clog
    //    << "program: " << opts::cmd_parser.getProgramName() << endl
    //    << "version: " << opts::cmd_parser.getVersion() << endl
    //    << "args: " << opts::cmd_parser.getOrigArgv() << endl;
    if (opts::ls + opts::id + opts::rw + opts::ed + opts::ev + opts::fq == 0)
    {
        opts::ls.set(true);
    }
    else if (opts::ls + opts::id + opts::rw + opts::ed + opts::ev + opts::fq > 1)
    {
        cerr << "at most one of --ls/--id/--rw/--ed/--ev/--fq must be given" << endl;
        exit(EXIT_FAILURE);
    }
    cout.precision(opts::float_prec);
    real_main();
}
