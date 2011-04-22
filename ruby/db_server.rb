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
  property :injection_time,     Float
  property :oil_pressure,       Float
  property :con_per_h,          Float
  property :con_per_100km,      Float
end

class OtherData
  include DataMapper::Resource

  property :id,                 Serial
  property :created_at,         DateTime
  property :temp_engine,        Float
  property :temp_air_intake,    Float
  property :voltage,            Float
end

DataMapper.auto_upgrade!

file = "/tmp/nerdobd2_socket"

File.unlink if File.exist?(file)

server = UNIXServer.new(file)

while (true) do
  socket = server.accept

  while(line = socket.gets) do
    data = JSON.parse(line)

    EngineData.create(data) if data['rpm']
    OtherData.create(data) if data['voltage']
  end
end

