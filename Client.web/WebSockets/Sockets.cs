using System.Net.WebSockets;
using System.Text;

namespace Client.Web
{
    public class Sockets
    {
        public static Sockets Instance
        {
            get
            {
                if (LocalInstance == null)
                    LocalInstance = new Sockets();

                return LocalInstance;
            }
        }
        public static ClientWebSocket Socket
        {
            get
            {
                if (LocalSocket == null)
                    LocalSocket = new ClientWebSocket();

                return LocalSocket;
            }
        }
        private static ClientWebSocket LocalSocket;
        private static Sockets LocalInstance;

        public void ConnectAsync(Uri uri, CancellationToken cancellationtoken)
        {
             Socket.ConnectAsync(uri, cancellationtoken);
        }

        public void SendMessageAsync(string message, CancellationToken cancellationtoken)
        {
            if (Socket.State == WebSocketState.Open)
            {
                var buffer = new ArraySegment<byte>(Encoding.UTF8.GetBytes(message + "|"));
              Socket.SendAsync(buffer, WebSocketMessageType.Text, true, cancellationtoken);
            }
        }

        public async Task<string> ReceiveMessageAsync(CancellationToken cancellationtoken)
        {
            var buffer = new ArraySegment<byte>(new byte[4096]);
            var result = await Socket.ReceiveAsync(buffer, cancellationtoken);

            if (result.MessageType == WebSocketMessageType.Text)
            {
                return Encoding.UTF8.GetString(buffer.Array, 0, result.Count);
            }

            return null;
        }

        public async Task CloseAsync(WebSocketCloseStatus closestatus, string statusdescription, CancellationToken cancellationtoken)
        {
            await Socket.CloseAsync(closestatus, statusdescription, cancellationtoken);
            Socket.Dispose();
        }
    }
}
