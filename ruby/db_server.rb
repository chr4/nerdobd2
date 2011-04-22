#!/usr/bin/ruby1.9.1

require 'socket'

require 'rubygems'
require 'json'

require 'dm-core'
require 'dm-migrations'
require 'dm-validations'
require 'dm-timestamps'

DataMapper::Logger.new($stdout, :debug)
#DataMapper.setup(:default, 'sqlite:///tmp/nerdobd.sqlite3')
DataMapper.setup(:default, 'sqlite::memory:')


class EngineData
  include DataMapper::Resource

  property :id,                 Serial
  property :created_at,         DateTime
  property :rpm,                Float
  property :speed,              Float
  property :injectionTime,      Float
  property :conPerH,            Float
  property :conPer100km,        Float
end

#DataMapper.finalize
DataMapper.auto_upgrade!

file = "/tmp/nerdobd2_socket"

File.unlink if File.exist?(file) and File.socket?(file)

server = UNIXServer.new(file)

while (true) do
socket = server.accept

while(line = socket.gets) do
  data = JSON.parse(line)
  puts data
  EngineData.create(data);
end
end

