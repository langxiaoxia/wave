import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;

import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Logger;

public class App {

    public static void main(String[] arg) throws Exception {
        Logger logger = Logger.getLogger("s3-hls-server");
        logger.info("start s3-hls-server http://localhost:8081/replay.m3u8?camera_id=ipc1&start=2023-10-20%2008:01:01&end=2023-10-20%2008:02:02.");
        HttpServer server = HttpServer.create(new InetSocketAddress(8081), 0);
        server.createContext("/test", new TestHandler());
        server.createContext("/replay.m3u8", new ReplayHandler());
        server.start();
    }

    static class TestHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange exchange) throws IOException {
            String response = "hello world";
            exchange.sendResponseHeaders(200, 0);
            OutputStream os = exchange.getResponseBody();
            os.write(response.getBytes());
            os.close();
        }
    }

    static class ReplayHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange exchange) throws IOException {
            Map<String, String> params = queryToMap(exchange.getRequestURI().getQuery());
            String camera_id = params.get("camera_id");
            String start_time = params.get("start");
            String end_time = params.get("end");
            String response = "replay.m3u8: camera_id=" + camera_id + ", start=" + start_time + ", end=" + end_time;
            exchange.sendResponseHeaders(200, 0);
            OutputStream os = exchange.getResponseBody();
            os.write(response.getBytes());
            os.close();
        }
    }

    public static Map<String, String> queryToMap(String query) {
        Map<String, String> result = new HashMap<>();
        for (String param : query.split("&")) {
            String[] entry = param.split("=");
            if (entry.length > 1) {
                result.put(entry[0], entry[1]);
            } else {
                result.put(entry[0], "");
            }
        }
        return result;
    }
}
