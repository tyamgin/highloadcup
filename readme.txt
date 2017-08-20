# Cerate image from Dockerfile 
$ sudo docker build -t dumb .

# For run server
$ sudo docker run --rm -p 8080:8080 -t dumb

# Submit
$ sudo docker login stor.highloadcup.ru
Username: tyamgin@mail.ru                           # мой email
Password: 4412c99fa435                                          # мой секретный ключ из личного кабинета
Login Succeeded                                             # на highloadcup.ru

$ sudo docker tag dumb stor.highloadcup.ru/travels/quetzal_climber  # тут disabled_cat - это мой личный
$ sudo docker push stor.highloadcup.ru/travels/quetzal_climber      # репозиторий со страницы с задачей





# Send post query
$ curl --data "{\"id\":245555,\"email\":\"foobar@mail.ru\",\"first_name\":\"Маша\",\"last_name\":\"Пушкина\",\"gender\":\"f\",\"birth_date\":365299200}" http://localhost:8080/users/new

$ curl --data "{invalid jsic}" http://localhost:8080/users/new

$ curl --data "{\"gender\":\"m\"}" http://localhost:8080/users/23


$ curl --data "{\"gender\":\"m\"}" http://localhost:8080/users/new?query_id=4


curl --data "{\"country\":\"Палау\"}" http://localhost:8080/locations/46
curl --data "{\"id\":232323,\"user\":23,\"location\":46,\"mark\":3,\"visited_at\":232323}" http://localhost:8080/visits/new


curl --data "{\"country\":\"\u0418\u0437\u0440\u0430\u0438\u043b\u044c\"}" http://localhost:8080/locations/46

