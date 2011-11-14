#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using namespace boost;

struct Option_Error: public std::exception
{
  Option_Error() {};
};

struct Options
{
  bool quiet;
  int max_depth;
  std::string format;
  bool sorted;
  std::string sort;
  std::string exclude;

  struct Stat
  {
    bool user;
    bool uid;
    bool group;
    bool gid;
    bool size;
    bool perm;
    bool inode;
    bool hardlinks;
    bool atime;
    bool mtime;
    bool ctime;

    Stat()
      : user(false),
	uid(false),
	group(false),
	gid(false),
	size(false),
	perm(false),
	inode(false),
	hardlinks(false),
	atime(false),
	mtime(false),
	ctime(false)
    {
    }
  };

  Stat stat;

  Options()
    : quiet(false),
      max_depth(-1),
      format("%p %l %u %g %h %M %N"),
      sorted(false),
      sort(),
      exclude()
  {
  }
};

Options options;

void set_stats(const char& c)
{
  switch (c) // fuuuu...
    {
    case 'n':
    case 'b':
    case 'e':
    case 'E':
      break;
    case 's':
      {
	options.stat.size = true;
	break;
      }
    case 'u':
      {
	options.stat.user = true;
	break;
      }
    case 'U':
      {
	options.stat.uid = true;
	break;
      }
    case 'g':
      {
	options.stat.group = true;
	break;
      }
    case 'G':
      {
	options.stat.gid = true;
	break;
      }
    case 'p':
    case 'P':
      {
	options.stat.perm = true;
	break;
      }
    case 'i':
      {
	options.stat.inode = true;
	break;
      }
    case 'l':
      {
	options.stat.hardlinks = true;
	break;
      }
    case 'a':
    case 'A':
      {
	options.stat.atime = true;
	break;
      }
    case 'm':
    case 'M':
      {
	options.stat.mtime = true;
	break;
      }
    case 'c':
    case 'C':
      {
	options.stat.ctime = true;
	break;
      }
    default:
      throw Option_Error();
    }
}

void read_sort()
{
  for ( std::string::const_iterator it = options.sort.begin();
	it != options.sort.end(); it++ )
    set_stats(*it);

  // TODO building sort tree
}

void read_format()
{
  for ( std::string::const_iterator it = options.format.begin();
	it != options.format.end(); it++ )
    if ( *it == '%' )
      {
	if ( ++it != options.format.end() )
	  set_stats( *it );
	else
	  throw Option_Error();
      }
}

void display_path( filesystem::path path )
{
  std::cout << path << std::endl;
}

void list_content_unsorted( filesystem::path p, int sublevels )
{
  try
    {
      for ( filesystem::directory_iterator dir(p);
	    dir != filesystem::directory_iterator(); dir++ )
	{
	  display_path(dir->path());
	  if ( filesystem::is_directory(*dir) )
	    if ( sublevels )
	      list_content_unsorted( *dir, sublevels - 1 );
	}
    }
  catch (const filesystem::filesystem_error& ex)
    {
      if ( ! options.quiet )
	std::cerr << p << ": permission denied" << std::endl;
    }
}

int main(int argc, char* argv[])
{
  try
    {
      program_options::options_description generic("Usage: fls [-x GLOB] [-f FMT] DIR...");
      generic.add_options()
	("help,h", "print this message")
	("format,f",
	 program_options::value<std::string>(&options.format),
	 "output format\n\
%n filename      %N raw filename\n\
%b basename      %B raw basename\n\
%u user          %u uid\n\
%g group         %G gid\n\
%s size          %h human size\n\
%p permstring    %P octal perm\n\
%i inode number  %l number of hardlinks\n\
%e extension     %E name without extension\n\
%a iso atime     %A epoch atime\n\
%m iso mtime     %M epoch mtime\n\
%c iso ctime     %C epoch ctime\n\
%F indicator (*/=|)  %_ column alignment")
	("sort,s",
	 program_options::value<std::string>(&options.sort),
	 "sort the files in order of given following arguments\n\
n filename   b basename    s size\n\
u user       U uid         g group\n\
i inode      l number of hardlinks\n\
e extension  E name without extension\n\
a atime      m mtime       c ctime")
	("reverse,r", "reverse display order")
	("max-depth,m", program_options::value<int>(&options.max_depth),
	 "max depth")
	("exclude,x",
	 program_options::value<std::string>(&options.exclude),
	 "exclude GLOB (** for recursive *)")
	("quiet,q", "don't show trivial error messages");

      program_options::options_description hidden("");
      hidden.add_options()
	("file", program_options::value<std::string>(), "file to stat");

      program_options::options_description cmdline_options;
      cmdline_options.add(generic).add(hidden);

      program_options::positional_options_description posdesc;
      posdesc.add("file", -1);

      program_options::variables_map vm;
      program_options::store(program_options::command_line_parser(argc, argv).
			     options(cmdline_options).positional(posdesc).run(),
			     vm);
      program_options::notify(vm);

      if (vm.count("help"))
	{
	  std::cout << generic << std::endl;
	  return 1;
	}

      if (vm.count("quiet"))
	options.quiet = true;

      if (vm.count("sort"))
	options.sorted = true;

      /* reading options to structures */
      read_sort();
      read_format();

      /* splitting at begin for optimization */
      if (vm.count("sort"))
	{
	  std::cout << "sorted check" << std::endl;
	  return 0;
	}
      else // not sorted
	{
	  filesystem::path p = vm["file"].as<std::string>();
	  if ( filesystem::is_regular_file( p ) )
	    {
	      display_path( p );
	      return 0;
	    }
	  else if ( filesystem::is_directory(p) )
	    {
	      list_content_unsorted(p, options.max_depth);
	      return 0;
	    }
	  else
	    return 1;
	}
    }
  catch( std::exception& e )
    {
      std::cerr << e.what() << std::endl;
      return 1;
    }

}
