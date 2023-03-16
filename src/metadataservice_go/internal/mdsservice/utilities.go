package mdsservice

func (s *Service) GetIP() string {
	return s.processor.GetConnectionInformation()
}
