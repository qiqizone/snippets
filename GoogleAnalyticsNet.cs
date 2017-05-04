static void Track(Guid clientId, string trackingId = "UA-97098474-1")
{
    System.Threading.Tasks.Task.Factory.StartNew(() =>
    {

        var postData = "v=1&t=event"
                     + "&tid=" + trackingId
                     + "&ds=app&dp=asdf"
                     + "&an=" + System.Reflection.Assembly.GetEntryAssembly().FullName
                     + "&u1=" + System.Globalization.CultureInfo.CurrentUICulture.Name
                     + "&cid=" + clientId.ToString().Replace("{", "").Replace("}", "")
                     + "&ec=" + "UI"
                     + "&ea=" + "Command"
                     + "&el=" + "CmdSave";

        var data = Encoding.ASCII.GetBytes(postData);

        var request = (HttpWebRequest)WebRequest.Create("http://www.google-analytics.com/collect");
        request.Method = "POST";
        request.ContentType = "application/x-www-form-urlencoded";
        request.ContentLength = data.Length;

        using (var stream = request.GetRequestStream())
            stream.Write(data, 0, data.Length);

        var response = (HttpWebResponse)request.GetResponse();
        using (response.GetResponseStream())
        { };
    });
}
