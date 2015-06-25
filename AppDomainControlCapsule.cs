    public class AppDomainControlCapsule
    {
        public FrameworkElement CreateControl(Func<FrameworkElement> createControl)
        {
            var ads = new AppDomainSetup();
            _domain = AppDomain.CreateDomain("ComHelper", null, ads);
            var domainBridge = (IDomainBridge)_domain.CreateInstanceAndUnwrap(Assembly.GetExecutingAssembly().FullName, "AutoHide.DomainBridge");

            var contract = domainBridge.CreateControl(createControl);

            var element = System.AddIn.Pipeline.FrameworkElementAdapters.ContractToViewAdapter(contract);
            return element;
        }

        private AppDomain _domain;
    }

    public class DomainBridge : MarshalByRefObject, IDomainBridge
    {
        private FrameworkElement _element;

        public Func<FrameworkElement> CreateControlFunc
        {
            get;
            set;
        }

        public INativeHandleContract CreateControl(Func<FrameworkElement> createControl)
        {
            //_element = new DataGrid() { Columns = { new DataGridTextColumn() { Header = "appdomain" } } };
            _element = createControl();
            return System.AddIn.Pipeline.FrameworkElementAdapters.ViewToContractAdapter(_element);
        }
    }

    public interface IDomainBridge
    {
        INativeHandleContract CreateControl(Func<FrameworkElement> createControl);
    }
