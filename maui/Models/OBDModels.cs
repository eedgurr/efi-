using System.Collections.ObjectModel;

namespace OBD2Tool.Models
{
    public class OBDData
    {
        public string PID { get; set; }
        public string Description { get; set; }
        public double Value { get; set; }
        public string Unit { get; set; }
        public DateTime Timestamp { get; set; }
    }

    public class PIDData
    {
        public string ID { get; set; }
        public string Description { get; set; }
        public string Unit { get; set; }
        public double MinValue { get; set; }
        public double MaxValue { get; set; }
        public Func<byte[], double> Formula { get; set; }
    }

    public class DTCCode
    {
        public string Code { get; set; }
        public string Description { get; set; }
        public DTCSeverity Severity { get; set; }
        public DateTime Timestamp { get; set; }
    }

    public enum DTCSeverity
    {
        Low,
        Medium,
        High,
        Critical
    }
}
