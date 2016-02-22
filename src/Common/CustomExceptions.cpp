class NotImplementedException : public std::logic_error
{
public:
    char const * what() const override { return "Case or Function not yet implemented."; }
}; 
